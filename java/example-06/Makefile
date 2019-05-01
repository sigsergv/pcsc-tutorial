default: help

help:
	@echo "Available commands:"
	@echo "    make test"
	@echo "    make issue-card"

%.class: %.java
	javac $<

compile: Util.class Test.class IssueCard.class

test: compile
	java Test

issue-card: compile
	java IssueCard
