#!/usr/bin/perl

use CSS::Minifier qw(minify);
use JavaScript::Minifier qw(minify);
use HTML::Packer;

use strict;

print "Started...\n";

my @files = qw(
ajax.js
bs.010.css
bs.025.css
index.html
index_body.html
script.js
styles.css
u.01.css
u.02.css
);

my @plain_files = qw(
reload.png
loader.gif
icon.png
);

my $outDir = "data";

foreach my $file (@files) {
	print "\n############################################### [$file]\n";
	if( -e $file) {
		if($file =~ /\.css$/) {
			print "--->>> css-file found, minify $file ...";
			open(INFILE, '<' . $file) or die "$file (1) not openned!\n";
			open(OUTFILE, '>../' . $outDir . '/' . $file) or die "$file.tmp (2) not openned!\n";
			CSS::Minifier::minify(input => *INFILE, outfile => *OUTFILE);
			close(INFILE);
			close(OUTFILE);
			print "\t\tdone\n";
		}
		if($file =~ /\.js$/) {
			print "--->>> js-file found, minify $file ...";
			open(INFILE, '<' . $file) or die "$file (1) not openned!\n";
			open(OUTFILE, '>../' . $outDir . '/' . $file) or die "$file.tmp (2) not openned!\n";
			JavaScript::Minifier::minify(input => *INFILE, outfile => *OUTFILE);
			close(INFILE);
			close(OUTFILE);
			print "\t\tdone\n";	
		}
		if($file =~ /\.html$/) {
			print "--->>> html-file found, minify $file ...";
			open(INFILE, '<' . $file) or die "$file (1) not openned!\n";
			my $html;
			while(my $line = <INFILE>) {
				$line =~ s/\n/ /;
				$html .= $line;
			}
			close(INFILE);
			my $packer = HTML::Packer->init();
			my $opt_html = $packer->minify(\$html, {
				remove_comments => 1,
				remove_newlines => 1
			});
			open(OUTFILE, '>../' . $outDir . '/' . $file) or die "$file (2) not openned!\n";
			print OUTFILE $opt_html;
			close(OUTFILE);
			print "\t\tdone\n";
		}
		my $cmd = "gzip -f ../$outDir/$file";
		print "--->>> $cmd ...";
		system($cmd);
		print "\t\tdone\n";
	} else {
		print "--->>> File $file not found!\n";
	}
}

#exit(0);

foreach my $file (@plain_files) {
	print "\n############################################### [$file]\n";
	if( -e $file) {
		my $cmd = "copy $file .." . '\\' . $outDir . '\\' . $file;
		print "$cmd ...";
		system($cmd);
		print "\t\tdone\n";
	} else {
		print "--->>> File $file not found!\n";
	}
}
