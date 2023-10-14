from http.server import BaseHTTPRequestHandler, HTTPServer
import cgi
from datetime import datetime

data_reservoir = []

content_length = 13
class RequestHandler(BaseHTTPRequestHandler):
    def do_POST(self):
        constant_length = int(self.headers['Content-Length'])
        post_data = self.rfile.read(content_length)
        print(f"Received data: {post_data.decode('utf-8')}")

        current_datetime = datetime.now().strftime("%Y-%m-%d %H:%M:%S")

        data_formatted = current_datetime + ' ' + post_data.decode('utf-8') + '\n'
        try:
            with open("data.txt", "a") as file:
                if data_reservoir:
                    for line in data_reservoir:
                        file.write(line)
                    data_reservoir.clear()
                    print("Reservoir dumped")
                file.write(data_formatted)
                file.close()
        except Exception as e:
            data_reservoir.append(data_formatted)
            print("Open file fail, data saved to reservoir")

        self.send_response(200)
        self.send_header("Content-type", "text/plain")
        self.end_headers()
        self.wfile.write(b"Data received and saved successfully")

def run_server():
    server_address = ('', 8000)
    httpd = HTTPServer(server_address, RequestHandler)
    print('Server is running on port 8000')
    httpd.serve_forever()

if __name__ == '__main__':
    run_server()
