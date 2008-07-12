program testa_if;

var v:integer;

procedure p;
begin
  v:=v+1;
end;

procedure q;
begin
  v:=v-1;
end;

function resto:integer;
begin
  resto := v - (2*(v/2));
end;

begin
  read(v);
  
{isso nao causa erros:
  if resto = 0 then begin
    p;
  end else begin 
    q;
  end;
}

{ mas isso causa (e nao deveria!) }
  if resto = 0 then
  
  
  
  
                                 p
  
  
  
  
  
   else 
   
   
   
   
   
   
   q;

  write(v);
end.
