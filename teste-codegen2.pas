program test2;

var x,y: integer;

begin
	read(x);
	
	if x > 10 then
	begin
		y := 0;
		y := y + 1;
	end else begin
		y := 1;
		y := y + 1;
	end;
	
	write(y);
end.
