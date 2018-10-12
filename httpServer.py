#reference: https://www.acmesystems.it/python_http
from http.server import BaseHTTPRequestHandler,HTTPServer #Python 3
import time
import csv

html_page = "\
<!DOCTYPE html>\
<html>\
<body>\
<h2>Click any buttons to generate data</h2><br>\
<form action=\"\" method=\"post\">\
    <button style=\"height:50px;width:200px\" name=\"btn1\" value=\"1\">1</button>\
</form>\
<form action=\"\" method=\"get\">\
    <button style=\"height:50px;width:200px\" name=\"btn2\" value=\"2\">2</button>\
</form><br>\
<form action=\"\" method=\"post\">\
    <button style=\"height:50px;width:200px\" name=\"btn3\" value=\"3\">3</button>\
</form>\
<form action=\"\" method=\"get\">\
    <button style=\"height:50px;width:200px\" name=\"btn4\" value=\"4\">4</button>\
</form><br>\
<form action=\"\" method=\"post\">\
    <button style=\"height:50px;width:200px\" name=\"btn5\" value=\"5\">5</button>\
</form>\
<form action=\"\" method=\"get\">\
    <button style=\"height:50px;width:200px\" name=\"btn6\" value=\"6\">6</button>\
</form><br>\
<form action=\"\" method=\"post\">\
    <button style=\"height:50px;width:200px\" name=\"btn7\" value=\"7\">7</button>\
</form>\
<form action=\"\" method=\"get\">\
    <button style=\"height:50px;width:200px\" name=\"btn8\" value=\"8\">8</button>\
</form><br>\
</body>\
</html>"

### HTTP Header columns being logged - Time,Source-IP,Method-Type,Host,User-Agent,Accept,Accept-Language,Accept-Encoding,Accept-Charset,Referer,Keep-Alive,Connection,Upgrade-Insecure-Requests,If-Modified-Since,If-None-Match,Cache-Control,Content-Length,Content-Type,Origin
###

f = open('malware.csv', 'a+')
wr = csv.writer(f, dialect='excel')
total_count = 1000
count = 0

class CustomHandler(BaseHTTPRequestHandler):

   def do_GET(self):
      print("GET === %s : from %s headers = %s, path = %s" % (self.date_time_string(time.time()), self.client_address[0], self.headers, self.path)) 
      # Log the header to csv
      resultHeadings = ["Host","User-Agent","Accept","Accept-Language","Accept-Encoding","Accept-Charset","Referer","Keep-Alive",\
"Connection","Upgrade-Insecure-Requests","If-Modified-Since","If-None-Match","Cache-Control","Content-Length",\
"Content-Type","Origin"]
      results = []
      results.append(time.time())                   #Time
      results.append(self.client_address[0])        #Source-IP
      results.append("GET")                         #Method-Type
      for heading in resultHeadings:
          if heading in self.headers:
              results.append(self.headers.get(heading))
          else:
              results.append("NULL")
      wr.writerow(results)
      global count
      count += 1
      if count >= total_count: raise Exception("Done")
      
      # Send the header
      self.send_response(200)
      self.send_header('Content-type','text/html')
      self.end_headers()
      
      # Send the html message
      self.wfile.write(html_page.encode()) #Python 3

      return

   def do_POST(self):
      content_length = self.headers.get('content-length') #python 3, python 2 uses getheader()
      length = 0
      if content_length: 
         if type(content_length) == list: length = int(content_length[0])
         length = int(content_length)
      postData = self.rfile.read(length).decode('utf-8')
      
      print("POST === %s : headers = %s, path = %s, data = %s" % (time.time(), self.headers, self.path, postData))
      
      # Log the header to csv
      resultHeadings = ["Host","User-Agent","Accept","Accept-Language","Accept-Encoding","Accept-Charset","Referer","Keep-Alive",\
"Connection","Upgrade-Insecure-Requests","If-Modified-Since","If-None-Match","Cache-Control","Content-Length",\
"Content-Type","Origin"]
      results = []
      results.append(time.time())                               #Time
      results.append(self.client_address[0])                    #Source-IP
      results.append("POST")                                    #Method-Type
      for heading in resultHeadings:
          if heading in self.headers:
              results.append(self.headers.get(heading))
          else:
              results.append("NULL")      
      wr.writerow(results)
      global count
      count += 1
      if count >= total_count: raise Exception("Done")
          
      #send back 204 No Content
      self.send_response(204)
      self.end_headers()
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
