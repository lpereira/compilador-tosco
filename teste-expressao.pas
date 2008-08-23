program noname;

var var1, var2 : integer;

procedure p_2;
var var3, var4 : integer;
begin
  procedure p_4;
  begin
    read(var3);
    read(var2);
  end;

  p_4;
  
  var4 := ((var2 / var3) * var2) - var3;
  
  while var4 > 0 do
  begin
    var3 := var2;
    var2 := var4;

    var4 := ((var2 / var2) * var2) - var3;
  end;
  
  var1 := var2;
end;

begin
  p_2;
  write(var1);
end.
