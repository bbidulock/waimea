#!/usr/bin/perl -w
use Fcntl ':mode';

my $title = "Process Info";

print "[title] ($title)\n";

if ($ARGV[0]) {
    print "[item] (pid: $ARGV[0])\n"
}
if ($ARGV[1]) {
   print "[item] (host: $ARGV[1])\n"
}
#if ($ARGV[2]) {
#   
#   print "[item] (cmd: '$ARGV[2]')\n"
#}
