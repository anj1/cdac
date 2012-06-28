#!/usr/bin/env python

import sys
import pygtk
import gtk
import gtk.glade
pygtk.require("2.0")

class Robie:
	
	def __init__(self):
		self.gladefile = "../../code/hello.glade"
		#self.wTree     = gtk.glade.XML(self.gladefile)
		builder = gtk.Builder()
		builder.add_from_file(self.gladefile)
		#builder.connect_signals(self)
		builder.connect_signals({"destroy": gtk.main_quit})
		self.window = builder.get_object("wndMain")
		self.window.show()
		
		#self.window = self.wTree.get_widget("wndMain")
		#if(self.window):
		#	self.window.connect("destroy", gtk.main_quit)

if __name__ == "__main__":
	rb = Robie()
	gtk.main()
