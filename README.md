# Linux C++ file transfer

## Build

```bash
cmake -B build
cmake --build build -j
```

## Run

Server:

```bash
./build/file-transfer -s [port]
```

Client:

```bash
./build/file-transfer -c ./file.bin [port]
```

Порт по умолчанию: `5000`. Порт можно задать последним аргументом.

## Protocol

```text
HEADER
  magic
  version
  file_size
  CRC32
DATA
ACK (SUCCESS / ERROR)
```

Сервер использует TCP-сокет для прослушивания входящих подключений. Для каждого клиента accept() создаёт отдельный подключённый сокет. Все сокеты отслеживаются одним экземпляром Linux epoll.

Полученные данные записываются во временные файлы с расширением .hex.part. После проверки размера и контрольной суммы CRC32 файлы переименовываются в .hex.
