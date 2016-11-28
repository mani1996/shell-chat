import cmd
import socket
import sys
import json


class ChatClient(cmd.Cmd):
	''' The main class for Shell Chat Client '''

	prompt = '>>> '
	intro = '''

	SSSS  H  H  EEEE L     L        CCCC  H  H    AA   TTTTT
	S     H  H  E    L     L        C     H  H   A  A    T
	SSSS  HHHH  EEEE L     L        C     HHHH   AAAA    T
	   S  H  H  E    L     L        C     H  H   A  A    T
	SSSS  H  H  EEEE LLLL  LLLL     CCCC  H  H   A  A    T

	Chat with your buddies the programmer way :)

	Type "help" to view list of commands. 
	Type "help <command>" to view command description.

	NOTE : Automatic refresh isn't available as of now. 
	Press Enter to check for new messages

	An app by Manikantan Narasimhan
	'''
	doc_header = 'Commands'
	ruler = '-'


	def __init__(self, config):
		cmd.Cmd.__init__(self)
		self.Socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM, 0)
		self.Socket.connect((config['address'] ,int(config['port'])))
		username = config['username']
		self.do_name(username, True)
		self.setPrompt(username)
		self.namesList = [username]


	def setPrompt(self, name):
		self.prompt = name + ':'


	def Request(self, request):
		self.Socket.send(json.dumps(request))
		data = self.Socket.recv(1024) # Assuming that response size is limited to 1024 bytes for now
		return data


	def emptyline(self):
		self.updateMessages()
		self.updateUsers()


	def default(self, line):
		self.stdout.write('ERROR : Unrecognised command "%s"\n'%line)
		self.updateMessages()
		self.updateUsers()


	def do_psend(self,line):
		'Send message privately to an user. Syntax is "psend <username> <message>"'
		self.updateMessages()
		self.updateUsers()

		spacePos = line.find(' ')
		assert(0 < spacePos < len(line)-1)

		requestData = {
			'type' : 'psend',
			'username' : line[:spacePos],
			'message' : line[spacePos+1:]
		}

		response = json.loads(self.Request(requestData))

		if 'error' in response:
			print
			print 'ERROR: ' + response['error']
			print
		else:
			print
			print response['message']
			print


	def complete_psend(self, text, *ignore):
		return [name for name in self.namesList if name.startswith(text)]


	def do_gsend(self,line):
		'Send message to the chatroom. Syntax is "gsend <message>"'
		self.updateMessages()
		self.updateUsers()

		requestData = {
			'type' : 'gsend',
			'message' : line
		}

		response = self.Request(requestData)
		print response


	def complete_gsend(self, text, *ignore):
		return [name for name in self.namesList if name.startswith(text)]


	def do_name(self,name,initial = False):
		'Change your name. Only alphabets, digits and underscores are allowed in the name'
		self.updateMessages()
		self.updateUsers()

		requestData = {
			'type' : 'name',
			'newName' : name.strip()
		}

		response = json.loads(self.Request(requestData))

		if 'error' in response:
			if initial:
				raise Exception(response['error'])
			else:
				print
				print 'ERROR: ' + response['error']
				print
		else:
			self.setPrompt(response['name'])
			if not initial:
				print
				print response['response']
				print


	def do_ol(self,line,echo = True):
		'Find users who are online'
		self.updateMessages()

		requestData = {
			'type' : 'ol', 
		}

		response = json.loads(self.Request(requestData))

		if echo:
			print 
			print '-------------'
			print 'ONLINE USERS:'
			print '-------------'
			for user in response:
				print user
			print
			print

		self.namesList = response


	def do_messages(self,line):
		'View all the received messages'
		self.updateMessages()
		self.updateUsers()

		requestData = {
			'type' : 'messages',
			'username' : line.strip()
		}

		response = json.loads(self.Request(requestData))

		if 'error' in response:
			print
			print 'ERROR: ' + response['error']
			print
		else:
			header = 'MESSAGES FROM ' + response['sender'] + ':'
			print
			print '-' * len(header)
			print header
			print '-' * len(header)

			for message in response['messages']:
				print message

			print
			print


	def complete_messages(self, text, *ignore):
		return [name for name in self.namesList if name.startswith(text)]


	def do_EOF(self, line):
		self.updateMessages()
		return True


	def updateUsers(self):
		self.do_ol('', echo = False) # This will update list of online users


	def updateMessages(self):
		self.Socket.setblocking(False)
		try:
			data = self.Socket.recv(1024)
			print
			print 'NEW MESSAGE!\n------------\n' + data
			print

		except Exception:
			pass	# No messages received
		self.Socket.setblocking(True)


def setConfig(config):
	requiredParams = {'username', 'address', 'port'}
	providedParams = set(config.keys())

	if not requiredParams.issubset(providedParams):
		print 'Following settings are required in config.json : ' + ','.join(requiredParams)
		return

	ChatClient(config).cmdloop()

if __name__ == '__main__':
	try:
		config = json.loads(open(sys.argv[1]).read())
	except:
		config = json.loads(open('default.json').read())
	setConfig(config)