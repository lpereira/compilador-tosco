program test;

var x,y,z,w: integer;

begin
	y := 5;
	x:= 10 + y;
	z := x + y - 1;
	
	read(w);
	
	x := x + w * y - z;
	
	write(x);
	write(y);
	write(z);
	write(w);
end.
