BIN:=esp_controller
LDGLAGS:=-Wl,--copy-dt-needed-entries -lubus
.PHONY: all clean

all: $(BIN)

$(BIN): esp_controller.c
	$(CC) -o $@ $^ $(LDGLAGS)

clean:
	rm -f $(BIN)
