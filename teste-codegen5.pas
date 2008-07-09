program test;

var x,y,z,w: integer;

function test_function : integer;
begin
	test_function := 5 * x;
end;

begin
	y := 5;
	x:= 10 + y;
	z := x + y - test_function;
	
	read(w);
	
	x := x + w * y - z;
	
	write(x);
	write(y);
	write(z);
	write(w);
end.
