{ este programa verifica se os offsets das variaveis globais e locais
  sao calculados corretamente }
program testeallocvar;

var
  variavel1, variavel2, variavel3 : integer;
  variavel4, variavel5 : boolean;

procedure teste;
var
  variavel6, variavel7 : integer;
begin
  variavel6 := variavel1 + variavel2 / variavel3;
  variavel7 := variavel6 * 5;
  variavel6 := variavel7 / 2;
  
  variavel1 := variavel6 * 2;
end;

begin
  variavel1 := 20909;
  variavel2 := 5;
  variavel3 := 1;
  variavel4 := true;
  variavel5 := false;
  
  teste;
  
  write(variavel1);
end.
