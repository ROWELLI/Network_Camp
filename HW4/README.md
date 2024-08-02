# 숙제#4
### 자동 완성 기능 기반 Search Engine 구현

- 서버의 Listen 포트번호와 검색어 데이터베이스 파일을 command line 
argument 로 넣어 프로그램을 실행한다. 
- ./search_server 9090 data.txt
- 파일로부터 검색어와 검색어의 검색횟수를 읽어 온다.
- 클라이언트에서 검색어를 입력하면 서버는 연관검색어를 검색횟수에
대해 내림차순 정렬하여 표시한다.  
- 클라이언트에서 검색어 입력시 검색어를 문자별로 인식하여 Enter를
치지 않더라도 실시간으로 검색어가 출력되게 한다. 
- 연관검색어는 최대 10개까지만 출력
- 연관검색어 출력시 검색어에 해당되는 부분은 임의의 색깔을 사용한다. 

서버 입력

![image](https://github.com/user-attachments/assets/85d2d8ec-ffc9-4a11-950b-319badbfbb7a)

서버에서는 보내는 값을 표시한다.

![image](https://github.com/user-attachments/assets/1afdff85-89e7-4cc0-a56c-91abc1b9c3c0)


클라이언트 입력
- 검색하고자 하는 문자를 입력한다.

![image](https://github.com/user-attachments/assets/0acdc960-0726-44d8-9777-121b7bb81589)
