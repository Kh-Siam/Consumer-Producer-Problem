run:
	@touch commodity
	@g++ -o consumer consumer.cpp
	@g++ -o producer producer.cpp

clean:
	@rm commodity consumer producer