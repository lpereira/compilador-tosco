program test;

var x,y: integer;

begin
	read(x);
	
	y:=0;
	while x > 0 do begin
		x := x - 1;
		y := y + 2;
	end;
	
	write(y);
end.
