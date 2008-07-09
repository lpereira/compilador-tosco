program test;

var x,y,z,w: integer;

procedure proc_test;
var j: integer;
begin
	j := x * 2;
	x:= j + 4 * j;
end;

begin
	y := 5;
	x:= 10 + y;
	z := x + y - 1;
	
	read(w);
	
	x := x + w * y - z;
	proc_test;
	
	write(x);
	write(y);
	write(z);
	write(w);
end.
