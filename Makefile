compile:
	g++ -g 5stage.cpp -o 5stage
	g++ -g 5stage_bypass.cpp -o 5stage_bypass
	g++ -g 79stage.cpp -o 79stage
	g++ -g 79stage_bypass.cpp -o 79stage_bypass

run_5stage:
	./5stage input.asm

run_5stage_bypass:
	./5stage_bypass input.asm

run_79stage:
	./79stage input.asm

run_79stage_bypass:
	./79stage_bypass input.asm

clean:
	rm 5stage
	rm 5stage_bypass
	rm 79stage
	rm 79stage_bypass