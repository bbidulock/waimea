#!/usr/bin/perl -w
use Fcntl ':mode';

my $script_location = $0;
my $offset = 0;
my $proc_dir = "/proc";
my $pid = 0;
my $host = "";
my $user = "";
$user = $ENV{USER};
chomp(my $hostname = qx(hostname));
my $forcepid = 0;
my $file = 0;
my $list_length = 20;

get_opt();

if ($forcepid) {
    if ($hostname eq $host && $pid) {
        pid_info();
    } else {
       print "[title] (process info)\n";
       print "[item] (process info for this window could not be retrived)\n";
    }
} else {
   if ($pid) {
       pid_info();     
   } else {
       list_proc();
   }
}

sub list_proc {
#    my $cmdline = 0;
    my $proc_name = 0;
    my $i = $offset;
    my $owner = "";
    my $printed = 0;
    my @output;

    print "[title] (process list)\n";

    opendir(DIR, $proc_dir) || die "can't opendir $proc_dir: $!";
    @proc_list = grep { (/^[^\.].*/ || /^\.\..*/) && /^\d+$/} readdir(DIR);
    closedir DIR;

#    print "$_\n" foreach @proc_list;


    while (($pid = $proc_list[$i]) && $printed <= $list_length) {
	open(FILE, "$proc_dir\/$pid\/status") || die "can't opendir $proc_dir: $!";
	($proc_name = (split(':', <FILE>))[1]) =~ s/^\s*(.*)\n/$1/;
#	if ($user eq 
	while ($_ = <FILE>) {
	    if (/^Uid\:\s*(\d+)\s+/) {
		$owner = getpwuid $1;
	    }
	}
	close FILE;
	
	if (($user && ($user eq 'all' || $user eq 'ALL')) ||
	    ($owner && $user && ($owner eq $user))) {
	    if ($printed < $list_length) {
		push @output, "[sub] ($pid - $proc_name) <!$script_location -pid $proc_list[$i]>\n";
	    }
	    else {
		$i--;
	    }
	    $printed++;
	}
	$i++;
    }

    if ($proc_list[$i]) {
	$_ = $i;
	print "[sub] (more...) <!$script_location -offset $_ -user $user>\n";
    }

    print @output;
}

sub pid_info {
    my $name = "";
    my @procinfo;
    my $cmdline = "";
    my $priority = "";
    my $msize = "";
    my $mlck = "";
    my $mrss = "";
    my $mdata = "";
    my $mstk = "";
    my $mexe = "";
    my $mlib = "";
    my $pid_err = 0;
    
    open(FILE, "$proc_dir\/$pid\/stat") || pid_err();
    $_ = <FILE>;
    @procinfo = split(/ /,$_);
    close FILE;
    $procinfo[1] =~ m/^\((.*)\)/;
    $name = $1;
    $priority = $procinfo[18];

    open(FILE, "$proc_dir\/$pid\/status") || die "can't opendir $proc_dir: $!";
    while ($_ = <FILE>) {
       if ($_ =~ m/^State.*\((\w*)\)$/) {
          $state = $1;
          $state =~ s/\(/\\\(/;
          $state =~ s/\)/\\\)/;
          $state =~ s/\t/ /;
          $state =~ s/\s+/ /;
       }
       if ($_ =~ m/^VmSize/) {          
          $_ =~ m/.*:\t*\s*(.*)$/;          
          $msize = $1;
       }
       if ($_ =~ m/^VmLck/) {
          $_ =~ m/.*:\t*\s*(.*)$/;
          $mlck = $1;
       }
       if ($_ =~ m/^VmRSS/) {
          $_ =~ m/.*:\t*\s*(.*)$/;
          $mrss = $1; 
       }
       if ($_ =~ m/^VmData/) {
          $_ =~ m/.*:\t*\s*(.*)$/;
          $mdata = $1;
       }
       if ($_ =~ m/^VmStk/) {
          $_ =~ m/.*:\t*\s*(.*)$/;
          $mstk = $1;
       }
       if ($_ =~ m/^VmExe/) {
          $_ =~ m/.*:\t*\s*(.*)$/;
          $mexe = $1;
       }
       if ($_ =~ m/^VmLib/) {
          $_ =~ m/.*:\t*\s*(.*)$/;
          $mlib = $1;
       }
    }        

    if (open(FILE, "$proc_dir\/$pid\/cmdline")) {
        $_ = <FILE>;        
        while ($_ =~ m/\0/) {
            $_ =~ s/\0/\\\" \\\"/;
        }
        $_ =~ m/(.*).{3}$/;
        $cmdline = "\\\"$1";
        close FILE;
    }

    print "[title] ($pid - $name)\n";
    
    print "[sub] (state \\\($state\\\)) <state_sub>\n";
    print "[start] (state_sub)\n";
    print "  [title] (set state)\n";
    print "  [item] (stop) {kill -SIGSTOP $pid}\n";
    print "  [item] (continue) {kill -SIGCONT $pid}\n";
    print "[end]\n";        
            
    print "[sub] (memory \\\($msize\\\)) <mem_sub>\n";
    print "[start] (mem_sub)\n";
    print "  [title] (memory usage)\n";
    print "  [item] (size: $msize)\n";
    print "  [item] (lck: $mlck)\n";
    print "  [item] (rss: $mrss)\n";
    print "  [item] (data: $mdata)\n";
    print "  [item] (stk: $mstk)\n";
    print "  [item] (exe: $mexe)\n";
    print "  [item] (lib: $mlib)\n";
    print "[end]\n";
    
    print "[sub] (priority \\\($priority\\\)) <prio>\n";
    print "[start] (prio)\n";
    print "  [title] (set priority)\n";
    print "  [item] (increase by 1) {renice +1 $pid}\n";
    print "  [item] (0 \\\(base\\\)) {renice 0 $pid}\n";
    print "  [item] (5) {renice 5 $pid}\n";
    print "  [item] (10) {renice 10 $pid}\n";
    print "  [item] (15) {renice 15 $pid}\n";    
    print "  [item] (20 \\\(Idle\\\)) {renice 20 $pid}\n";
    print "[end]\n";

    print "[submenu] (send signal)\n";
    print "  [item] (send sighup) {kill -HUP $pid}\n";
    print "  [item] (send sigint) {kill -INT $pid}\n";
    print "  [item] (send sigkill) {kill -KILL $pid}\n";
    print "[end]\n";

    if ($cmdline ne "") {
        print "[item] (restart) {kill $pid && $cmdline}\n";
        print "[item] (spawn new) {$cmdline}\n";    
    }
}

sub print_directory {
    my @output = "";
    my $mode;
    opendir(DIR, $dir) || die "can't opendir $dir: $!";
    @dir_list = grep { /^[^\.].*/ || /^\.\..*/} readdir(DIR);
    closedir DIR;
    
    $i = 0;
    while ($dir_list[$i+$offset] && (($i == $max_menu_length-1 && !$dir_list[$i+$offset+1]) ||
				     $i < $max_menu_length-1)) {
	$mode = (stat("$dir\/$dir_list[$i+$offset]"))[2];
	if ($mode & S_IFDIR) {
	    push  @output, print_sub("$dir_list[$i+$offset]\/", '0',
				     "$script_location -dir \\\"$dir/$dir_list[$i+$offset]\\\"");
	}
	if ($mode & S_IFREG) {
	    if ($mode & S_IEXEC) {
		push  @output, print_sub("$dir_list[$i+$offset]",
					 "\\\"$dir/$dir_list[$i+$offset]\\\"",
					 "$script_location -file \\\"$dir/$dir_list[$i+$offset]\\\"");
	    }
	    else {
		$_ = $dir_list[$i+$offset];
		/.*\.(.+)/;
		if ($1 && $mimes{$1} && $mimes{$1}[0]) {
		    $_ = $mimes{$1}[0];
		}
		else {
		    $_ = $mimes{'ALL'}[0];
		}
		if ( $1 && ($listing{'ALL'}{'sub'} || ($listing{$1} && $listing{$1}{'sub'}))) {
		    push  @output, print_sub("$dir_list[$i+$offset]",
					     "0", #"$_ \\\"$dir/$dir_list[$i+$offset]\\\"",
					     "$script_location -file \\\"$dir/$dir_list[$i+$offset]\\\"");
		}
		else {
		    push  @output, print_item("$dir_list[$i+$offset]",
					      "$_ \\\"$dir/$dir_list[$i+$offset]\\\"");
		}
	    }
	}
	$i++;
    }

    
    if ($dir_list[$i+$offset]) {
	$_ = $offset + $max_menu_length -1;
	unshift  @output, print_sub("more...", undef, "$script_location -dir \\\"$dir\\\" -offset $_");
    }
    
    $_[0] = $offset + $i;
    $_[1] = $offset + 1;
    unshift  @output, print_title("$_[1] - $_[0]");
    
    print @output;
}
#print "$_\n" foreach @dir_list;




#print $file_offset, $dir, $file;




sub get_opt {
    my $i = 0;
    while ($ARGV[$i+1] && ($_ = $ARGV[$i])) {
	if (/\-offset/) {
	    $offset = $ARGV[$i+1];
	}
	if (/\-pid/) {
	    $pid = $ARGV[$i+1];
	}
	if (/\-proc_dir/) {
	    $proc_dir = $ARGV[$i+1];
	}
	if (/\-user/) {
	    $user = $ARGV[$i+1];
	}
   if (/\-host/) {
      $host = $ARGV[$i+1];
   }
	$i++;
    }
    $i = 0;
    while ($_ = $ARGV[$i]) {
        $i++;
        if (/\-host/) {
            $forcepid = 1;
        }
    }
}

sub print_item {
    my $return_string = "[item]";
    if ($_[0]) {
	$return_string = "$return_string ($_[0])";
    }
    if ($_[1]) {
	$return_string = "$return_string {$_[1]}";
    }
    $return_string = "$return_string\n";
    return $return_string;
}

sub print_title {
    my $return_string = "[title]";
    if ($_[0]) {
	$return_string = "$return_string ($_[0])";
    }
    $return_string = "$return_string\n";
    return $return_string;
}


sub print_sub {
    my $return_string = "[sub]";
    if ($_[0]) {
	$return_string = "$return_string ($_[0])";
    }
    if ($_[1]) {
	$return_string = "$return_string {$_[1]}";
    }
    if ($_[2]) {
	$return_string = "$return_string \<\!$_[2]\>";
    }
    $return_string = "$return_string\n";
    return $return_string;
}

sub pid_err {
    print "[title] ($pid)\n";
    print "[item] (no process with pid $pid on this host)\n";
    exit;
}
                               
