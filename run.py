print("Server Launcher (c) Growtopia Noobs")
import sys
httpd=None
if sys.version_info[0] is 3:
    import http.server
    class ServerHandler(http.server.SimpleHTTPRequestHandler):
        def do_POST(self):
            self.send_response(200)
            self.end_headers()
            self.wfile.write(str.encode("server|127.0.0.1\nport|17091\ntype|1\n#maint|Mainetrance message (Not used for now) -- Growtopia Noobs\n\nbeta_server|beta.growtopiagame.com\nbeta_port|27003\n\nbeta_type|1\nmeta|localhost\nRTENDMARKERBS1001"))
    server_address = ('', 80)
    httpd = http.server.HTTPServer(server_address, ServerHandler)
    print("HTTP server is running! Don't close this or GT will be unable to connect!")
    httpd.serve_forever()
else:
    print("Use Python 3!")