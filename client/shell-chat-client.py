import cmd
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


	def __init__(self, port, username):
		cmd.Cmd.__init__(self)
		self.Socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM, 0)
		self.Socket.connect(('localhost',int(port)))
		self.do_name(username, True)
		self.setPrompt(username)


	def setPrompt(self, name):
		self.prompt = name + ':'


	def emptyline(self):
		self.refresh()


	def default(self, line):
		self.stdout.write('ERROR : Unrecognised command "%s"\n'%line)
		self.refresh()


	def do_psend(self,line):
		'Send message privately to an user. Syntax is "psend <username> <message>"'
		self.refresh()
		self.Socket.send('psend ' + line)
		response = self.Socket.recv(1024)
		print response


	def do_gsend(self,line):
		'Send message to the chatroom. Syntax is "gsend <message>"'
		self.refresh()
		self.Socket.send('gsend ' + line)
		response = self.Socket.recv(1024)
		print response


	def do_name(self,name,initial = False):
		'Change your name. Only alphabets, digits and underscores are allowed in the name'
		self.refresh()
		name = name.split()[0]
		self.Socket.send('name ' + name)
		response = self.Socket.recv(1024)
		if not initial:
			print response

		if response == 'ERROR : username already exists!' :
			raise Exception('Username already in use!')
		else:
			self.setPrompt(name)


	def do_ol(self,line):
		'Find users who are online'
		self.refresh()
		self.Socket.send('ol')
		response = self.Socket.recv(1024)
		print response


	def do_messages(self,line):
		'View all the received messages'
		self.refresh()
		self.Socket.send('messages ' + line)
		response = self.Socket.recv(1024)
		print response


	def do_EOF(self, line):
		self.refresh()
		return True


	def refresh(self):
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