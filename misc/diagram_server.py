from http.server import BaseHTTPRequestHandler, HTTPServer
from urllib.parse import parse_qs
import subprocess as sp
import os

def page(src="", content=""):
    return f"""
<html>
    <head>
        <meta charset="utf-8" />
        <title>Noja Diagram Server</title>
        <style>
        * {{
            margin: 0;
            padding: 0;
            outline: 0;
        }}
        body {{
            width: 100%;
            height: 100%;
        }}
        #left-panel,
        #right-panel {{
            width: calc(50% - 20px);
            height: calc(100% - 20px);
            float: left;
            padding: 10px;
        }}
        #controls {{
            width: 100%;
            height: 30px;
            overflow: auto;
        }}
        textarea {{
            width: 100%;
            height: calc(100% - 40px);
            padding: 10px;
            margin-top: 10px;
            font-size: 28px;
        }}
        input {{
            height: 30px;
            padding: 0 10px;
            float: right;
        }}
        #left-panel {{
            background-color: #eee;
        }}
        #right-panel {{
            overflow: auto;
        }}
        svg {{
            margin: auto 100px;
        }}
        </style>
    </head>
    <body>
        <div id="left-panel">
            <form method="post" action="/">
                <div id="controls">
                    <input type="submit" value="Draw AST">
                </div>
                <textarea id="message" name="message" rows="4" cols="50" placeholder="[Noja code goes here!]">{src}</textarea><br>
            </form>
        </div>
        <div id="right-panel">
            {content}
        </div>
    </body>
</html>
"""

class SimpleHTTPRequestHandler(BaseHTTPRequestHandler):
    def do_GET(self):
        self.send_response(200)
        self.send_header('Content-type', 'text/html')
        self.end_headers()
        self.wfile.write(page().encode('utf-8'))

    def do_POST(self):
        content_length = int(self.headers['Content-Length'])
        post_data = self.rfile.read(content_length).decode('utf-8')
        parsed_data = parse_qs(post_data)

        self.send_response(200)
        self.send_header('Content-type', 'text/html')
        self.end_headers()

        if 'message' not in parsed_data:
            self.wfile.write(page().encode('utf-8'))
            return

        message = parsed_data['message'][0]

        src = "/tmp/src.noja"
        tmp = "/tmp/tmp.dot"
        dst = "/tmp/dst.svg"

        open(src, "wb").write(bytes(message, 'ascii'))

        cmd = f"noja --diagram-ast -o {tmp} {src}"
        output = sp.Popen(cmd, shell=True, stderr=sp.PIPE).stderr.read().decode('utf-8')

        if not os.path.isfile(tmp):
            self.wfile.write(page(message, output).encode('utf-8'))
            return

        cmd = f"dot -Tsvg -o {dst} {tmp}"
        output = sp.Popen(cmd, shell=True, stdout=sp.PIPE).stdout.read().decode('utf-8')

        if not os.path.isfile(dst):
            self.wfile.write(page(message, output).encode('utf-8'))
            return

        svg = open(dst, "rb").read().decode('utf-8')

        os.remove(src)
        os.remove(tmp)
        os.remove(dst)

        self.wfile.write(page(message, svg).encode('utf-8'))

def run(server_class=HTTPServer, handler_class=SimpleHTTPRequestHandler, port=8000):
    server_address = ('', port)
    httpd = server_class(server_address, handler_class)
    print(f"Server avviato su http://localhost:{port}")
    httpd.serve_forever()

if __name__ == '__main__':
    run()