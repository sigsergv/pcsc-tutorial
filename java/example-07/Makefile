default: run

%.class: %.java
	javac $<

compile: ReadCard.class WriteCard.class Util.class

read: compile
	java ReadCard

write: compile
	java WriteCard