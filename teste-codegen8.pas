program visibilidade;

var x:integer;

procedure p1;
begin
  procedure proca;
  begin
    read(x);
    x := x * 2;
  end;
  
  procedure procb;
  begin
    write(x);
  end;
  
  proca;
  procb;
end;

procedure p2;
begin
  procedure proca;
  begin
    read(x);
    x := x / 2;
  end;
  
  procedure procb;
  begin
    write(x);
  end;
  
  proca;
  procb;
end;

begin
  p1;
  p2;
end.
