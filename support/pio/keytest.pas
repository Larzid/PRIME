program keytest;

uses Crt;

var key : Char;
begin
repeat
  key := readkey;
  writeln (ord (key), ' ', key)
until key = #27
end.
