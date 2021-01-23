//*                                *
//*  ___  ___ _ ____   _____ _ __  *
//* / __|/ _ \ '__\ \ / / _ \ '__| *
//* \__ \  __/ |   \ V /  __/ |    *
//* |___/\___|_|    \_/ \___|_|    *
//*                                *
//===- lib/http/server.cpp ------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//===----------------------------------------------------------------------===//
/// \file server.cpp
/// \brief Implements the top-level HTTP server functions.
#include "pstore/http/server.hpp"

// Standard library includes
#include <cstring>
#include <thread>

// OS-specific includes
#ifdef _WIN32
#    include <ws2tcpip.h>
#else
#    include <netdb.h>
#    include <sys/socket.h>
#endif

// Local includes
#include "pstore/http/headers.hpp"
#include "pstore/http/net_txrx.hpp"
#include "pstore/http/request.hpp"
#include "pstore/http/serve_dynamic_content.hpp"
#include "pstore/http/serve_static_content.hpp"
#include "pstore/http/wskey.hpp"

namespace {

    using socket_descriptor = pstore::socket_descriptor;

    template <typename Sender, typename IO>
    pstore::error_or<IO> cerror (Sender sender, IO io, pstore::gsl::czstring const cause,
                                 unsigned const error_no, pstore::gsl::czstring const shortmsg,
                                 pstore::gsl::czstring const longmsg) {
        static constexpr auto crlf = pstore::http::crlf;
        static constexpr auto server_name = pstore::http::server_name;

        std::ostringstream content;
        content << "<!DOCTYPE html>\n"
                   "<html lang=\"en\">"
                   "<head>\n"
                   "<meta charset=\"utf-8\">\n"
                   "<title>"
                << server_name
                << " Error</title>\n"
                   "</head>\n"
                   "<body>\n"
                   "<h1>"
                << server_name
                << " Web Server Error</h1>\n"
                   "<p>"
                << error_no << ": " << shortmsg
                << "</p>"
                   "<p>"
                << longmsg << ": " << cause
                << "</p>\n"
                   "<hr>\n"
                   "<em>The "
                << server_name
                << " Web server</em>\n"
                   "</body>\n"
                   "</html>\n";
        std::string const & content_str = content.str ();
        auto const now = pstore::http::http_date (std::chrono::system_clock::now ());

        std::ostringstream os;
        // clang-format off
        os << "HTTP/1.1 " << error_no << " OK" << crlf
           << "Server: " << server_name << crlf
           << "Content-length: " << content_str.length () << crlf
           << "Connection: close" << crlf  // TODO remove this when we support persistent connections
           << "Content-type: " << "text/html" << crlf
           << "Date: " << now << crlf
           << "Last-Modified: " << now << crlf
           << crlf
           << content_str;
        // clang-format on
        return pstore::http::send (sender, io, os);
    }

    // initialize_socket
    // ~~~~~~~~~~~~~~~~~
    pstore::error_or<socket_descriptor> initialize_socket (in_port_t const port_number) {
        using eo = pstore::error_or<socket_descriptor>;

        log (pstore::logger::priority::info, "initializing on port: ", port_number);
        socket_descriptor fd{::socket (AF_INET, SOCK_STREAM, 0)};
        if (!fd.valid ()) {
            return eo{pstore::http::get_last_error ()};
        }

        int const optval = 1;
        if (::setsockopt (fd.native_handle (), SOL_SOCKET, SO_REUSEADDR,
                          reinterpret_cast<char const *> (&optval), sizeof (optval))) {
            return eo{pstore::http::get_last_error ()};
        }

        sockaddr_in server_addr{}; // server's addr.
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = htonl (INADDR_ANY); // NOLINT
        server_addr.sin_port = htons (port_number);       // NOLINT
        if (::bind (fd.native_handle (), reinterpret_cast<sockaddr *> (&server_addr),
                    sizeof (server_addr)) < 0) {
            return eo{pstore::http::get_last_error ()};
        }

        // Get ready to accept connection requests.
        if (::listen (fd.native_handle (), 5) < 0) { // allow 5 requests to queue up.
            return eo{pstore::http::get_last_error ()};
        }

        return eo{std::move (fd)};
    }


    pstore::error_or<std::string> get_client_name (sockaddr_in const & client_addr) {
        std::array<char, 64> host_name{{'\0'}};
        constexpr std::size_t size = host_name.size ();
#ifdef _WIN32
        using size_type = DWORD;
#else
        using size_type = std::size_t;
#endif //!_WIN32
        int const gni_err = ::getnameinfo (
            reinterpret_cast<sockaddr const *> (&client_addr), sizeof (client_addr),
            host_name.data (), static_cast<size_type> (size), nullptr, socklen_t{0}, 0 /*flags*/);
        if (gni_err != 0) {
            return pstore::error_or<std::string>{pstore::http::get_last_error ()};
        }
        host_name.back () = '\0'; // guarantee nul termination.
        return pstore::error_or<std::string>{std::string{host_name.data ()}};
    }


    // Here we bridge from the std::error_code world to HTTP status codes.
    void report_error (std::error_code const error, pstore::http::request_info const & request,
                       socket_descriptor & socket) {
        static constexpr auto crlf = pstore::http::crlf;
        static constexpr auto sender = pstore::http::net::network_sender;

        auto report = [&error, &request, &socket] (unsigned const code,
                                                   pstore::gsl::czstring const message) {
            cerror (sender, std::ref (socket), request.uri ().c_str (), code, message,
                    error.message ().c_str ());
        };

        log (pstore::logger::priority::error, "http error: ", error.message ());
        if (error.category () == pstore::http::get_error_category ()) {
            switch (static_cast<pstore::http::error_code> (error.value ())) {
            case pstore::http::error_code::bad_request: report (400, "Bad request"); return;

            case pstore::http::error_code::bad_websocket_version: {
                // From rfc6455: "The |Sec-WebSocket-Version| header field in the client's handshake
                // includes the version of the WebSocket Protocol with which the client is
                // attempting to communicate. If this version does not match a version understood by
                // the server, the server MUST abort the WebSocket handshake described in this
                // section and instead send an appropriate HTTP error code (such as 426 Upgrade
                // Required) and a |Sec-WebSocket-Version| header field indicating the version(s)
                // the server is capable of understanding."
                std::ostringstream os;
                os << "HTTP/1.1 426 OK" << crlf //
                   << "Sec-WebSocket-Version: " << pstore::http::ws_version << crlf
                   << "Server: " << pstore::http::server_name << crlf //
                   << crlf;
                std::string const & reply = os.str ();
                sender (socket, as_bytes (pstore::gsl::make_span (reply)));
            }
                return;

            case pstore::http::error_code::not_implemented:
            case pstore::http::error_code::string_too_long:
            case pstore::http::error_code::refill_out_of_range: break;
            }
        } else if (error.category () == pstore::romfs::get_romfs_error_category ()) {
            switch (static_cast<pstore::romfs::error_code> (error.value ())) {
            case pstore::romfs::error_code::enoent:
            case pstore::romfs::error_code::enotdir: report (404, "Not found"); return;

            case pstore::romfs::error_code::einval: break;
            }
        }

        // Some error that we don't know how to report properly.
        std::ostringstream message;
        message << "Server internal error: " << error.message ();
        report (501, message.str ().c_str ());
    }


    template <typename Reader, typename IO>
    pstore::error_or<std::unique_ptr<std::thread>>
    upgrade_to_ws (Reader & reader, IO io, pstore::http::request_info const & request,
                   pstore::http::header_info const & header_contents,
                   pstore::http::channel_container const & channels) {
        using return_type = pstore::error_or<std::unique_ptr<std::thread>>;
        using priority = pstore::logger::priority;
        PSTORE_ASSERT (header_contents.connection_upgrade && header_contents.upgrade_to_websocket);

        log (priority::info, "WebSocket upgrade requested");
        // Validate the request headers
        if (!header_contents.websocket_key || !header_contents.websocket_version) {
            log (priority::error, "Missing WebSockets upgrade key or version header.");
            return return_type{pstore::http::error_code::bad_request};
        }


        if (*header_contents.websocket_version != pstore::http::ws_version) {
            log (priority::error, "Bad Websocket version number requested");
            return return_type{pstore::http::error_code::bad_websocket_version};
        }


        // Send back the server handshake response.
        auto const accept_ws_connection = [&header_contents, &io] () {
            log (priority::info, "Accepting WebSockets upgrade");

            static constexpr auto crlf = pstore::http::crlf;
            std::string const date = pstore::http::http_date (std::chrono::system_clock::now ());
            std::ostringstream os;
            // clang-format off
            os << "HTTP/1.1 101 Switching Protocols" << crlf
               << "Server: " << pstore::http::server_name << crlf
               << "Upgrade: WebSocket" << crlf
               << "Connection: Upgrade" << crlf
               << "Sec-WebSocket-Accept: " << pstore::http::source_key (*header_contents.websocket_key) << crlf
               << "Date: " << date << crlf
               << "Last-Modified: " << date << crlf
               << crlf;
            // clang-format on

            // Here I assume that the send() IO param is the same as the Reader's IO parameter.
            return pstore::http::send (pstore::http::net::network_sender, std::ref (io), os);
        };

        auto server_loop_thread = [&channels] (Reader && reader2, socket_descriptor io2,
                                               std::string const uri) {
            PSTORE_TRY {
                constexpr auto ident = "websocket";
                pstore::threads::set_name (ident);
                pstore::create_log_stream (ident);

                log (priority::info, "Started WebSockets session");

                PSTORE_ASSERT (io2.valid ());
                ws_server_loop (std::move (reader2), pstore::http::net::network_sender,
                                std::ref (io2), uri, channels);

                log (priority::info, "Ended WebSockets session");
            }
            // clang-format off
            PSTORE_CATCH (std::exception const & ex, { //clang-format on
                log (priority::error, "Error: ", ex.what ());
            })
            // clang-format off
            PSTORE_CATCH (..., { // clang-format on
                log (priority::error, "Unknown exception");
            })
        };

        // Spawn a thread to manage this WebSockets session.
        auto const create_ws_server = [&reader, server_loop_thread,
                                       &request] (socket_descriptor & s) {
            PSTORE_ASSERT (s.valid ());
            return return_type{pstore::in_place,
                               new std::thread (server_loop_thread, std::move (reader),
                                                std::move (s), request.uri ())};
        };

        PSTORE_ASSERT (io.get ().valid ());
        return accept_ws_connection () >>= create_ws_server;
    }

#ifndef NDEBUG
    template <typename BufferedReader>
    bool input_is_empty (BufferedReader const & reader, socket_descriptor const & fd) {
        if (reader.available () == 0) {
            return true;
        }
#    ifdef _WIN32
        // TODO: not yet implemented for Windows.
        (void) fd;
        return true;
#    else
        // There should be nothing in the buffered-reader or the input socket waiting to be read.
        errno = 0;
        std::array<std::uint8_t, 256> buf;
        ssize_t const nread = recv (fd.native_handle (), buf.data (),
                                    static_cast<int> (buf.size ()), MSG_PEEK /*flags*/);
        int const err = errno;
        if (nread == -1) {
            log (pstore::logger::priority::error, "error:", err);
        }
        return nread == 0 || nread == -1;
#    endif
    }
#endif

    // wait_for_connection
    // ~~~~~~~~~~~~~~~~~~~
    pstore::error_or<socket_descriptor> wait_for_connection (socket_descriptor const & parentfd) {
        using return_type = pstore::error_or<socket_descriptor>;

        using priority = pstore::logger::priority;

        sockaddr_in client_addr{}; // client address.
        auto clientlen = static_cast<socklen_t> (sizeof (client_addr));
        socket_descriptor childfd{::accept (parentfd.native_handle (),
                                            reinterpret_cast<struct sockaddr *> (&client_addr),
                                            &clientlen)};
        if (!childfd.valid ()) {
            return return_type{pstore::http::get_last_error ()};
        }

        // Determine who sent the message.
        PSTORE_ASSERT (clientlen == static_cast<socklen_t> (sizeof (client_addr)));
        pstore::error_or<std::string> ename = get_client_name (client_addr);
        if (!ename) {
            return return_type{pstore::http::get_last_error ()};
        }
        log (priority::info, "Connection from ", ename.get ());
        return return_type{std::move (childfd)};
    }

} // end anonymous namespace

namespace pstore {
    namespace http {

        int server (romfs::romfs & file_system, gsl::not_null<server_status *> const status,
                    channel_container const & channels,
                    std::function<void (in_port_t)> notify_listening) {
            using priority = logger::priority;

            error_or<socket_descriptor> const eparentfd = initialize_socket (status->port ());
            if (!eparentfd) {
                log (priority::error, "opening socket: ", eparentfd.get_error ().message ());
                return 0;
            }

            socket_descriptor const & parentfd = eparentfd.get ();
            status->set_real_port_number (parentfd);

            log (priority::info, "starting server-loop on port ", status->port ());

            std::vector<std::unique_ptr<std::thread>> websockets_workers;
            notify_listening (status->port ());

            for (auto expected_state = server_status::http_state::initializing;
                 status->listening (expected_state);
                 expected_state = server_status::http_state::listening) {

                // Wait for a connection request.
                pstore::error_or<socket_descriptor> echildfd = wait_for_connection (parentfd);
                if (!echildfd) {
                    log (priority::error,
                         "wait_for_connection: ", echildfd.get_error ().message ());
                    continue;
                }
                socket_descriptor & childfd = *echildfd;

                // Get the HTTP request line.
                auto reader = make_buffered_reader<socket_descriptor &> (net::refiller);

                PSTORE_ASSERT (childfd.valid ());
                pstore::error_or_n<socket_descriptor &, request_info> eri =
                    read_request (reader, std::ref (childfd));
                if (!eri) {
                    log (priority::error,
                         "Failed reading HTTP request: ", eri.get_error ().message ());
                    continue;
                }
                childfd = std::move (std::get<0> (eri));
                request_info const & request = std::get<1> (eri);
                log (priority::info, "Request: ",
                     request.method () + ' ' + request.version () + ' ' + request.uri ());

                // We only currently support the GET method.
                if (request.method () != "GET") {
                    cerror (pstore::http::net::network_sender, std::ref (childfd),
                            request.method ().c_str (), 501, "Not Implemented",
                            "httpd does not implement this method");
                    continue;
                }

                // Respond appropriately based on the request and headers.
                auto const serve_reply =
                    [&] (socket_descriptor & io2,
                         header_info const & header_contents) -> std::error_code {
                    if (header_contents.connection_upgrade &&
                        header_contents.upgrade_to_websocket) {

                        pstore::error_or<std::unique_ptr<std::thread>> p = upgrade_to_ws (
                            reader, std::ref (childfd), request, header_contents, channels);
                        if (p) {
                            websockets_workers.emplace_back (std::move (*p));
                        }
                        return p.get_error ();
                    }

                    if (!details::starts_with (request.uri (), dynamic_path)) {
                        return serve_static_content (pstore::http::net::network_sender,
                                                     std::ref (io2), request.uri (), file_system)
                            .get_error ();
                    }

                    return serve_dynamic_content (pstore::http::net::network_sender, std::ref (io2),
                                                  request.uri ())
                        .get_error ();
                };

                // Scan the HTTP headers.
                PSTORE_ASSERT (childfd.valid ());
                std::error_code const err = read_headers (
                    reader, std::ref (childfd),
                    [] (header_info io, std::string const & key, std::string const & value) {
                        return io.handler (key, value);
                    },
                    header_info ()) >>= serve_reply;

                if (err) {
                    // Report the error to the user as an HTTP error.
                    report_error (err, request, childfd);
                }

                PSTORE_ASSERT (input_is_empty (reader, childfd));
            }

            for (std::unique_ptr<std::thread> const & worker : websockets_workers) {
                worker->join ();
            }

            return 0;
        }

    } // end namespace http
} // end namespace pstore
