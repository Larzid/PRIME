program tablemaker;
{$LONGSTRINGS OFF}
{$LINKLIB c}
{$LINKLIB fl}
{$LINK tablemaker.tab.o}
{$LINK lex.yy.o}
uses
   strutils, sysutils;

{.$DEFINE PRT_INST}

type
TDefinition       = record
   template       : SmallInt;
   num_pairs      : Word;
   short          : String;
   names          : array of String;
   values         : array of String;
   inc_file       : Text;
   enm_file       : Text;
   enums          : Word
end;

const
STACKMAX          = 7;

var
num_defns         : Word;
defn_names        : array of String;
defn_data         : array of TDefinition;
inst_data         : array of String;
inst_id           : Word;
rec_level         : Word;
subr_begin        : Word; // keeping indices of subrange of elements
subr_end          : Word; // within a given record
rec_stack         : array[0..STACKMAX, 1..2] of Word;

{$DEFINE C}
{$IFDEF PAS}
const
LP                : Char = '(';
RP                : Char = ')';
SEP               : Char = ';';
SEP_PREFIX_NAME   : Char = '_';
INC_EXT           : String = '.inc';
ENM_EXT           : String = '.enm';
{$ENDIF}
{$IFDEF C}
const
LP                : Char = '{';
RP                : Char = '}';
SEP               : Char = ',';
SEP_PREFIX_NAME   : String = '';
INC_EXT           : String = '.cpp';
ENM_EXT           : String = '.h';
{$ENDIF}


function find_string(what : String; where : array of String) : Integer;
var i             : Integer;
    start, finish : Integer;
begin
if subr_begin = 0 then start := low(where) else start := subr_begin + 1;
if subr_end = 0 then finish := high(where) else finish := subr_end - 1;
for i := start to finish do
   if comparetext(what, where[i]) = 0 then
       exit(i);
find_string := -1
end;

// bison might return string consisting of more than one token but only the
// first is the intended argument. conv_arg extracts first token.
function conv_arg(arg : PChar) : String;
const SPACE       = [' ', #8, #10];
var i             : Integer;
begin
i := posset(SPACE, arg);
if i = 0
   then conv_arg := arg
   else conv_arg := copy(arg, 1, i - 1)
end;

procedure expand_arrays;
begin
if num_defns = length(defn_names) then // double space if needed
   begin
   setlength(defn_data, length(defn_names) * 2);
   setlength(defn_names, length(defn_names) * 2)
   end
end;

procedure begin_defn(name, short, variable, shortvar : PChar); cdecl; public;
const PROP_LEN    = 4;
var tmp_str       : String;
begin
expand_arrays;
// prepare record
defn_names[num_defns] := conv_arg(name);
defn_data[num_defns].template := -1;
defn_data[num_defns].short := short;
setlength(defn_data[num_defns].names, PROP_LEN);
setlength(defn_data[num_defns].values, PROP_LEN);
defn_data[num_defns].num_pairs := 0;
defn_data[num_defns].enums := 0;
// open files
with defn_data[num_defns] do
   begin
   assign(inc_file, defn_names[num_defns]+INC_EXT);
   rewrite(inc_file);
   tmp_str := defn_names[num_defns];
   {$IFDEF PAS}
   writeln(inc_file, 'const ', variable, ' : array[low(E', tmp_str,
           ')..high(E', tmp_str, ')] of T', tmp_str, 'Base =');
   {$ENDIF}
   {$IFDEF C}
   writeln(inc_file, 'sh', tmp_str, ' ', variable, ' =');
   {$ENDIF}
   assign(enm_file, defn_names[num_defns]+ENM_EXT);
   rewrite(enm_file);
   {$IFDEF PAS}
   //writeln(enm_file, 'type');
   writeln(enm_file, 'N', defn_names[num_defns], ' =')
   {$ENDIF}
   {$IFDEF C}
   writeln(enm_file, '#ifndef '+ansiuppercase(defn_names[num_defns])+'_H');
   writeln(enm_file, '#define '+ansiuppercase(defn_names[num_defns])+'_H');
   writeln(enm_file, 'enum ', shortvar)
   {$ENDIF}
   end
end;

procedure add_name_val(new_name, new_value : PChar); cdecl; public; forward;
procedure begin_rec(name : PChar); cdecl; public;
begin
inc(rec_level);
add_name_val(name, nil)
end;

procedure add_name_val(new_name, new_value : PChar); cdecl; public;
begin
with defn_data[num_defns] do
   begin
   if num_pairs > high(names) then
      begin
      setlength(names, length(names) * 2);
      setlength(values, length(names) * 2)
      end;
   names[num_pairs] := conv_arg(new_name);
   values[num_pairs] := new_value;
   inc(num_pairs)
   end
end;

procedure end_rec; cdecl; public;
begin
dec(rec_level);
add_name_val(nil, nil)
end;

procedure end_defn; cdecl; public;
begin
inc(num_defns)
end;

procedure begin_templ(name, n_base : PChar); cdecl; public;
var base, i       : LongInt;
begin
expand_arrays;
// check if base exists
base := find_string(conv_arg(n_base), defn_names);
if base < 0 then
   begin
   writeln('Undefined base ('+conv_arg(n_base)+') for '+conv_arg(name));
   exit
   end;
defn_names[num_defns] := conv_arg(name);
// prepare record by assigning template index
defn_data[num_defns].template := base;
defn_data[num_defns].num_pairs := defn_data[base].num_pairs;
// name/value pairs are copied
setlength(defn_data[num_defns].names, length(defn_data[base].names));
setlength(defn_data[num_defns].values, length(defn_data[base].values));
for i := 0 to high(defn_data[num_defns].names) do
   defn_data[num_defns].names[i] := defn_data[base].names[i];
for i := 0 to high(defn_data[num_defns].values) do
   defn_data[num_defns].values[i] := defn_data[base].values[i]
end;

procedure find_rec(name : PChar; templ : Integer); cdecl; public;
var i, j          : LongInt;
    stack         : Word;
    curid         : Word;
begin
if rec_level = STACKMAX then
   begin
   writeln('find_rec failure: stack size exceeded when adding "', conv_arg(name), '"');
   exit
   end;
if templ = 0 then curid := inst_id else curid := num_defns;
with defn_data[curid] do
   begin
   i := find_string(conv_arg(name), names);
   if i < 0 then
      begin
      writeln('Record "'+conv_arg(name)+'" not found in "'+defn_names[num_defns]+'"');
      exit
      end;
   rec_stack[rec_level][1] := subr_begin;
   rec_stack[rec_level][2] := subr_end;
   inc(rec_level);
   subr_begin := i;
   // find end of record
   stack := 1;
   for j := i+1 to high(names) do
      begin
      if names[j] = '' then
         dec(stack)
      else if values[j] = '' then
         inc(stack);
      if stack = 0 then
         begin
         subr_end := j;
         break
         end
      end
   end;
   if stack <> 0 then
      begin
      writeln('find_rec failure: parent for "', conv_arg(name), '" not found in "'+defn_names[num_defns]+'"');
      exit
      end
end;

procedure override_name_val(name, new_value : PChar); cdecl; public;
var i             : LongInt;
begin
with defn_data[num_defns] do
   begin
   i := find_string(conv_arg(name), names);
   if i < 0 then
      begin
      writeln('Name "'+conv_arg(name)+'" not found in "'+defn_names[num_defns]+'"');
      writeln('Value requested: ', new_value);
      exit
      end;
   values[i] := new_value
   end
end;

procedure exit_rec; cdecl; public;
begin
if rec_level = 0 then
   begin
   writeln('exit_rec failure: no stack levels to climb');
   exit
   end;
dec(rec_level);
subr_begin := rec_stack[rec_level][1];
subr_end := rec_stack[rec_level][2]
end;

procedure end_templ; cdecl; public;
begin
end_defn
end;

procedure empty_inst(d_name, i_name : PChar); cdecl; public;
var i             : LongInt;
begin
{$IFDEF PRT_INST}writeln('empty_inst: ', d_name, ' ', i_name);{$ENDIF}
i := find_string(conv_arg(d_name), defn_names);
if i >= 0 then
   inst_id := i
else
   begin
   writeln('Undefined block "'+conv_arg(d_name)+'"');
   exit
   end;
if defn_data[i].enums > 0 then   // separate elements with comma
   writeln(defn_data[i].enm_file, ',')
else                             // first opening brace for array contents
   writeln(defn_data[i].enm_file, LP);
write(defn_data[i].enm_file, defn_data[i].short, SEP_PREFIX_NAME, i_name);
inc(defn_data[i].enums)
end;

procedure begin_inst(d_name, i_name : PChar); cdecl; public;
var i, j, len     : LongInt;
    templ         : LongInt;
begin
setlength(inst_data, 0);
i := find_string(conv_arg(d_name), defn_names);
if i >= 0 then
   inst_id := i
else
   begin
   writeln('Undefined block "'+conv_arg(d_name)+'"');
   exit
   end;
{$IFDEF PRT_INST}writeln(conv_arg(d_name), ' ', i_name);{$ENDIF}
// are we using template?
if defn_data[i].template >= 0
   then templ := defn_data[i].template
   else templ := i;
// prepare instance record
len := defn_data[templ].num_pairs;
setlength(inst_data, len);
if defn_data[templ].enums > 0 then
   // separate elements with comma
   begin
   writeln(defn_data[templ].inc_file, ',');
   writeln(defn_data[templ].enm_file, ',')
   end
else
   // first opening brace for array contents
   begin
   writeln(defn_data[templ].inc_file, LP);
   writeln(defn_data[templ].enm_file, LP)
   end;
writeln(defn_data[templ].inc_file, '   ', LP, '  // ', i_name);
write(defn_data[templ].enm_file, defn_data[templ].short, SEP_PREFIX_NAME, i_name);
inc(defn_data[templ].enums);
// set default values from template
for j := 0 to len - 1 do
   inst_data[j] := defn_data[inst_id].values[j]
end;

procedure add_inst_val(new_name, new_value : PChar); cdecl; public;
var i             : LongInt;
begin
i := find_string(conv_arg(new_name), defn_data[inst_id].names);
if i < 0 then
   begin
   writeln('Unknown field "'+conv_arg(new_name)+'" in definition of "'+defn_names[inst_id]+'".');
   writeln('Value requested: ', new_value);
   exit
   end;
inst_data[i] := new_value
end;

procedure end_inst; cdecl; public;
var i, templ      : Word;

   procedure indent;
   var i             : Word;
   begin
   for i := 0 to rec_level do
      write(defn_data[templ].inc_file, '   ')
   end;

begin
{$IFDEF PRT_INST}writeln('end;');{$ENDIF}
// are we using template?
if defn_data[inst_id].template >= 0
   then templ := defn_data[inst_id].template
   else templ := inst_id;
inc(rec_level);
with defn_data[inst_id] do
   for i := 0 to num_pairs - 1 do
      begin
      if names[i] = '' then
         begin
         dec(rec_level);
         indent;
         write(defn_data[templ].inc_file, RP);
         if (i = num_pairs - 1) or (names[i+1] = '')
            then writeln(defn_data[templ].inc_file)
            else writeln(defn_data[templ].inc_file, SEP)
         end
      else if inst_data[i] = '' then
         begin
         {$IFDEF PAS}
         indent;
         writeln(defn_data[templ].inc_file, names[i], ':');
         {$ENDIF}
         indent;
         inc(rec_level);
         writeln(defn_data[templ].inc_file, LP)
         end
      else
         begin
         indent;
         {$IFDEF PAS}
         write(defn_data[templ].inc_file, names[i], ': ');
         {$ENDIF}
         write(defn_data[templ].inc_file, inst_data[i]);
         if (i = num_pairs - 1) or (names[i+1] = '')
            then writeln(defn_data[templ].inc_file)
            else writeln(defn_data[templ].inc_file, SEP)
         end
      end;
write(defn_data[templ].inc_file, '   ', RP);
dec(rec_level)
end;

procedure init;
const INIT_LEN    = 128;
begin
setlength(defn_names, INIT_LEN);
setlength(defn_data, INIT_LEN);
num_defns := 0;
rec_level := 0;
subr_begin := 0;
subr_end := 0
end;

procedure done;
var i             : LongInt;
begin
for i := 0 to num_defns - 1 do
   if defn_data[i].template = -1 then // do not write templates
      begin
      writeln(defn_data[i].inc_file);
      writeln(defn_data[i].inc_file, RP, ';');
      close(defn_data[i].inc_file);
      writeln(defn_data[i].enm_file);
      writeln(defn_data[i].enm_file, RP, ';');
      {$IFDEF C}
      writeln(defn_data[i].enm_file, '#endif');
      {$ENDIF}
      close(defn_data[i].enm_file)
      end
end;

function yyparse : Integer; cdecl; external;

begin
init;
yyparse;
done
end.
