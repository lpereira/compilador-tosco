program test;

var x,y,z,w: integer;

function test_function : integer;
var
	teste: integer;
begin
	teste := 5 * x;
	
	if teste > 100 then test_function := teste + 1
	else test_function := teste - 1;
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
