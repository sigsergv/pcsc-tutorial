default: help

help:
	@echo "Available commands:"
	@echo "    make test"
	@echo "    make issue-card"

%.class: %.java
	javac $<

compile: Util.class Test.class IssueCard.class CheckBalance.class TopUpBalance.class Checkout.class RevokeCard.class

test: compile
	java Test

issue-card: compile
	java IssueCard

check-balance: compile
	java CheckBalance

top-up-balance: compile
	java TopUpBalance $(ADD)

checkout: compile
	java Checkout

revoke-card: compile
	java RevokeCard

