#!/usr/bin/perl
# Simple script to generate a 404 HTTP error

print "Status: 404 Not Found\r\n";
print "Content-type: text/html\r\n";
print "\r\n";

print "<html><body>";
print "This 404 error was intentionally generated by a test script.";
print "</html></body>";
