#!/usr/bin/perl -w

use strict;

use CGI;
use File::stat;

use constant CHUNK_SIZE_BYTES => 1024;

my $query = new CGI;

my $name = $query->param('name');
my $filesize = stat($name)->size;

# Get throttling rate, assuming parameter is in kilobytes per second.
my $kbPerSec = $query->param('throttle');
my $chunkPerSec = $kbPerSec * 1024 / CHUNK_SIZE_BYTES;

# Get MIME type if provided.  Default to video/mp4.
my $type = $query->param('type') || "video/mp4";

# Print HTTP Header, disabling cache.
print "Content-type: " . $type . "\n"; 
print "Content-Length: " . $filesize . "\n";
print "Cache-Control: no-cache\n";
print "\n";

open FILE, $name or die;
binmode FILE;
my ($data, $n);
my $total = 0;
while (($n = read FILE, $data, 1024) != 0) {
    print $data;

    $total += $n;
    if ($total >= $filesize) {
        last;
    }

    # Throttle if there is some.
    if ($chunkPerSec > 0) {
        select(undef, undef, undef, 1.0 / $chunkPerSec);
    }
}
close(FILE);
