import cmd
import re
import socket
import sys
import threading
import time


class ChatClient(cmd.Cmd):
	''' The main class for Shell Chat Client '''

	prompt = '>>> '
	intro = '''
	SHELL CHAT
	Chat with your buddies the programmer way :)

	Type "help" to view list of commands. 
	Type "help <command>" to view command description.

	NOTE : Automatic refresh isn't available as of now. 
	Press Enter to check for new messages
	'''
	doc_header = 'Commands'
	ruler = '-'


	@staticmethod
	def validUsername(name):
		pattern = re.compile(r'[a-zA-Z0-9_]+')
		match = pattern.match(name)
		return (match is not None) and (match.group() == name)  


	def __init__(self, port, username):
		cmd.Cmd.__init__(self)
		self.Socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM, 0)
		self.Socket.connect(('localhost',int(port)))
		self.do_name(username, True)
		self.setPrompt(username)
		self.namesList = []


	def setPrompt(self, name):
		self.prompt = name + ':'


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
		self.Socket.send('psend ' + line)
		response = self.Socket.recv(1024)
		print response


	def complete_psend(self, text, *ignore):
		return [name for name in self.namesList if name.startswith(text)]


	def do_gsend(self,line):
		'Send message to the chatroom. Syntax is "gsend <message>"'
		self.updateMessages()
		self.updateUsers()
		self.Socket.send('gsend ' + line)
		response = self.Socket.recv(1024)
		print response


	def complete_gsend(self, text, *ignore):
		return [name for name in self.namesList if name.startswith(text)]


	def do_name(self,name,initial = False):
		'Change your name. Only alphabets, digits and underscores are allowed in the name'
		self.updateMessages()
		self.updateUsers()
		name = name.split()[0]
		self.Socket.send('name ' + name)
		response = self.Socket.recv(1024)
		if not initial:
			print response

		if response == 'ERROR : username already exists!' :
			raise Exception('Username already in use!')
		else:
			self.setPrompt(name)


	def do_ol(self,line,echo = True):
		'Find users who are online'
		self.updateMessages()
		self.Socket.send('ol')
		response = self.Socket.recv(1024)
		if echo:
			print response
		words = response.split('\n')
		self.namesList = filter(lambda word : ChatClient.validUsername(word), words)


	def do_messages(self,line):
		'View all the received messages'
		self.updateMessages()
		self.updateUsers()
		self.Socket.send('messages ' + line)
		response = self.Socket.recv(1024)
		print response


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
			print 'NEW MESSAGE!\n' + data
		except Exception:
			pass	# No messages received
		self.Socket.setblocking(True)


if __name__ == '__main__':
	if len(sys.argv) > 2:
		ChatClient(sys.argv[1], sys.argv[2]).cmdloop()
	else:
		raise Exception('usage: greet-client.py <port> <username>')