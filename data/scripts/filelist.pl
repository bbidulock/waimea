#!/usr/bin/perl -w
use POSIX;

my $script_location = $0;
my $offset = 0;
my $dir = "$ENV{HOME}";
my $file = 0;
my $max_menu_length = 20;



my %mimes = ('ALL', ['xmms -p', 'xmms -e'],
	     'mp3', ['xmms -p', 'xmms -e'],
	     'avi', ['mplayer -quiet','då'],
	     'jpg', ['xv', 'Esetroot']);

my %listing = ('ALL', {'sub', '0'},
	       'mp3', {'sub', '1', 'size', '1'},
	       'mpg', {'sub', '1', 'size', '1'},
	       'avi', {'sub', '1', 'size', '1'},
	       'doc', {'sub', '1'});

get_opt();

if ($file) {
    print_file();
}
else {
    print_directory();
}



sub print_file {
    my @output = "";
    my @mode;
    my $size = 0;
    my $file_type;

    @mode = stat("$file");
    if (@mode) {
	$_ = $file;
	/.*\.(.+)/;
	$file_type = $1;
	if ($listing{$file_type}{size}) {
	    if ($mode[7] > 1073741824) {
		$_ = int ($mode[7] / 1073741824);
		$size = "size $_ G";
	    }
	    else {
		if ($mode[7] > 1048576) {
		    $_ = int ($mode[7] / 1048576);
		    $size = "size $_ M";
		}
		else {
		    if ($mode[7] > 1024) {
			$_ = int ($mode[7] / 1024);
			$size = "size $_ k";
		    }
		    else {
			$size = "size $mode[7]";
		    }
		}
	    }
	    
	    push  @output, print_item($size, "0");
	}
    }
    $_ = $file;
    /.*\/(.+)/;
    unshift  @output, print_title("$1");
    

    print @output;
}

sub print_directory {
    my @output = "";
    my $mode;
    opendir(DIR, $dir) || die "can't opendir $dir: $!";
    @dir_list = grep { /^[^\.].*/ } readdir(DIR);
    closedir DIR;
    
    $i = 0;
    while ($dir_list[$i+$offset] && (($i == $max_menu_length-1 && !$dir_list[$i+$offset+1]) ||
				     $i < $max_menu_length-1)) {
	$mode = (stat("$dir\/$dir_list[$i+$offset]"))[2];
	if (S_ISDIR($mode)) {
	    push  @output, print_sub("$dir_list[$i+$offset]\/", '0',
				     "$script_location -dir \\\"$dir/$dir_list[$i+$offset]\\\"");
	}
	if (S_ISREG($mode)) {
	    if ($mode & S_IXUSR) {
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
    unshift  @output, print_title("$dir");
    
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
	if (/\-dir/) {
	    $dir = $ARGV[$i+1];
	}
	if (/\-file/) {
	    $file = $ARGV[$i+1];
	}
	$i++;
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
