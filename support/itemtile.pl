#!/usr/bin/perl
my ( @items, $file, $last );
open $file, "<", "Items.txt" or die "problem - items";
@items = <$file>;
close $file;
$last = "";
$flav = "";
foreach ( @items ) {
    if ( /^\w+ (\w+)$/ ) {
        if ( $last ) {
            if ( $flav ) {
                print "   * $last ($flav)\n";
            } else {
                print "   * $last\n";
            }
        }
        $last = $1;
        $flav = "";
    }
    $flav = $1 if ( /flavor\s*kF(\w+)/ );
    if ( /tile_col\s*(\d+)/ ) {
        $last = "" if ( $1 );
    }
}
