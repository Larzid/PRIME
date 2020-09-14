#!/usr/bin/perl
my ( @tmp, @items, @ilks, @missing, $file );
open $file, "<", "ItemSet.cpp" or die "problem - ItemSet.cpp";
@tmp = <$file>;
close $file;

foreach ( @tmp ) {
    if ( /kObj/ ) {
        /(kObj\w*)/;
        chomp $_;
        push (@items, $1);
    }
}

open $file, "<", "ObjectIlk.h" or die "problem - ObjectIlk.h";
@tmp = <$file>;
close $file;

foreach ( @tmp ) {
    if ( /kObj/ and not /\=/ ) {
        /(kObj\w*)/;
        chomp $_;
        push (@ilks, $1);
    }
}

foreach my $ilk ( @ilks ) {
    my $flag = 1;
    foreach $item ( @items ) {
        if ( $item eq $ilk ) {
            $flag = 0;
            last;
        }
    }
    push (@missing, $ilk) if $flag == 1;
}

foreach ( @missing ) {
    print "$_\n";
}
