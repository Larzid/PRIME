#!/usr/bin/perl
my ( @monsters, $file );
open $file, "<", "Monsters.txt" or die "problem - monsters";
@monsters = <$file>;
close $file;
my $last;
foreach ( @monsters ) {
    if ( /MonsterIlk (\w+)/ ) {
        $last = $1;
    }
    if ( /\s*tile_col \s* (\d+)/ ) {
        print "   * " . $last . "\n" if ($1 == 0);
    }
}
