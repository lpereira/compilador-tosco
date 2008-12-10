all:
	cd compilador && make
	cd maquina-virtual && make
	cp compilador/csd .
	cp maquina-virtual/mvd .
	