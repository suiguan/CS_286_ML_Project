#reference: https://www.acmesystems.it/python_http
from http.server import BaseHTTPRequestHandler,HTTPServer #Python 3
import time


class CustomHandler(BaseHTTPRequestHandler):

   def do_GET(self):
      print("=== %s : headers = %s, path = %s" % (self.date_time_string(time.time()), self.headers, self.path))

      # Send the header
      self.send_response(200)
      self.send_header('Content-type','text/html')
      self.end_headers()

      # Send the html message
      htmlMsg= "Hello World!"

      self.wfile.write(htmlMsg.encode()) #Python 3

      return

if __name__ == '__main__':
   PORT_NUMBER = 8000
   try:
      #Create a web server with our handler
      server = HTTPServer(('127.0.0.1', PORT_NUMBER), CustomHandler)
      print('Started httpserver on port %d' % PORT_NUMBER)
      
      #Wait forever for incoming http requests
      server.serve_forever()

   except KeyboardInterrupt:
      server.socket.close()
