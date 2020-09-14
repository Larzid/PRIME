library pio;

uses Crt, Video;

procedure pio_init; cdecl;
begin
InitVideo
end;

procedure pio_exit; cdecl;
begin
DoneVideo
end;

procedure pio_clear; cdecl;
begin
ClearScreen
end;

procedure pio_gotoxy (x, y : Integer); cdecl;
begin
SetCursorPos (x, y)
end;

procedure pio_echo (x, y : Integer; c : Char); cdecl;
begin
VideoBuf^[x + y * ScreenWidth] := ord (c) + (textattr shl 8)
end;

procedure pio_draw (x, y : Integer; c : Char; fg, bg : Integer); cdecl;
begin
VideoBuf^[x + y * ScreenWidth] := ord (c) + (fg shl 8) + (bg shl 12)
end;

function pio_screen (x, y : Integer) : Integer; cdecl;
begin
pio_screen := VideoBuf^[x + y * ScreenWidth]
end;

procedure pio_string (x, y : Integer; s : PChar); cdecl;
var len, max, cnt : Integer;
begin
len := strlen (s) - 1;
max := x + len;
if max > ScreenWidth then max := ScreenWidth;
for cnt := x to max do
    VideoBuf^[cnt + y * ScreenWidth] := ord (s[cnt - x]) + (textattr shl 8)
end;

procedure pio_textattr (attr : Integer); cdecl;
begin
textattr := attr
end;

procedure pio_refresh; cdecl;
begin
UpdateScreen (false)
end;

function pio_get_width : Cardinal; cdecl;
begin
pio_get_width := ScreenWidth
end;

function pio_get_height : Cardinal; cdecl;
begin
pio_get_height := ScreenHeight
end;

function pio_read_key : Char; cdecl;
begin
pio_read_key := readkey
end;

exports
  pio_init, pio_exit, pio_clear, pio_refresh,
  pio_gotoxy, pio_echo, pio_draw, pio_screen,
  pio_string, pio_textattr,
  pio_get_width, pio_get_height, pio_read_key;

end.
