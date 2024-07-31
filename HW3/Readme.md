# 숙제#3
### ▪ Simple Remote Shell 프로그램 구현
- 클라이언트가 서버에 접속하여 서버의 디렉토리와 파일 정볼르 확인할 수 있는 프로그램을 작성하세요.
- 클라이언트가 서버에 접속하면, 서버는 프로그램이 실행되고 있는 디렉토리 위치 및 파일 정보(파일명 + 파일 크기)를 클라이언트에게 전달합니다.
- 클라이언트는 서버로부터 수신한 정보들을 출력합니다.
- 클라이언트는 서버의 디렉토리를 변경해가며 디렉토리 정보와 디렉토리 안에 있는 파일 정보(파일명 + 파일크기)를 확인할 수 있습니다.
- 클라이언트는 자신이 소유한 파일을 서버에게 업로드 할 수 있어야 합니다.
- 클라이언트가 서버에 접속하면 클라이언트는 서버 프로그램의 권한과 동일한 권한을 갖습니다. 즉, 서버 프로그램에게 혀용되지 않은 디렉토리와 파일은 클라이언트도 접근할 수 없습니다.
- 서버는 여러 클라이언트의 요청을 동시에 지원할 수 있어야 합니다.
-> I/O Multiplexing 사용!
- Makefile을 만들어서 컴파일 할 수 있어야 합니다.

1. ls

현재 디렉토리의 파일과 디렉토리를 보여줍니다.

![image](https://github.com/user-attachments/assets/707f9da2-2cdd-40ce-83fc-34b46237aebf)

2. cd \<dirname>

서버에서는 움직인 디렉토리를 볼 수 있습니다.

server

![image](https://github.com/user-attachments/assets/8e16b4ab-d16c-4ee0-9243-3957b3c118fc)

client

![image](https://github.com/user-attachments/assets/b36c4e9a-0320-4696-bac5-20243bbf9958)

3. ul

클라이언트의 파일을 서버에 옮길 수 있습니다.

![image](https://github.com/user-attachments/assets/5175bb4a-9ef7-4c98-9251-5634cccbefb8)

4. dl

서버에서 원하는 파일을 index로 클라이언트에 가져올 수 있습니다.

![image](https://github.com/user-attachments/assets/8ed4734c-203d-4d85-a9bf-0a53c752ede6)