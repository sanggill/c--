#include "widget.h"
#include "ui_widget.h"

Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{
    ui->setupUi(this);
    //감정 테스트를위한 이미지 백터에 경로 저장

    Image_Vector.append(":/new/prefix1/image/block4.jfif");
    Image_Vector.append(":/new/prefix1/image/butter4.jfif");
    Image_Vector.append(":/new/prefix1/image/cro3.jfif");
    Image_Vector.append(":/new/prefix1/image/image1.jpg");
    Image_Vector.append(":/new/prefix1/image/image2.jpg");
    Image_Vector.append(":/new/prefix1/image/missle2.jfif");
    Image_Vector.append(":/new/prefix1/image/room.png");
    Image_Vector.append(":/new/prefix1/image/stone4.jpeg");
    Image_Vector.append(":/new/prefix1/image/tigerai2.jpeg");

    initialize();
}

void Widget::initialize()
{
    // 고정된 IP 주소와 포트 번호 설정
    QHostAddress hostAddress("10.10.21.111");
    int portNumber = 26000;

    tcpServer = new QTcpServer(this);
    py_tcpSocket=new QTcpSocket(this);
    if (!tcpServer->listen(hostAddress, portNumber))
    {
        QMessageBox::critical(this, tr("TCP Server"), tr("서버를 시작할 수 없습니다. 에러메세지 : %1.").arg(tcpServer->errorString()));
        close();
        return; // 실패시 종료
    }
    else
    {
        ui->server_text->setText(tr("서버 동작 중\n\n""IP : %1\n""PORT : %2\n").arg(hostAddress.toString()).arg(portNumber));

        //서버 동작 시 DB연결
        db = QSqlDatabase::addDatabase("QMYSQL");
        db.setHostName("10.10.21.111"); //접속 IP
        db.setDatabaseName("3team");    // DB명
        db.setPort(3306);               // Port 번호
        db.setUserName("sanggil");      // 접속자 이름
        db.setPassword("1111");         // 접속자 비밀번호

    if(db.open())
    {
        ui->server_text->append("DB Open Success");
    }

    else
    {
        ui->server_text->append("DB not Open ");
        qDebug() << "DB Open Error : " << db.lastError().text();
    }

    //파이썬 서버 연결 함수 이동
      PyServer_Connect();
    }

    // 새로운 연결 요청이 tcpServer에 들어오면, 슬롯 함수 실행
    connect(tcpServer, SIGNAL(newConnection()), this, SLOT(NEWConnection()));

    //파이썬 데이터 읽기
    connect(py_tcpSocket, SIGNAL(readyRead()), this, SLOT(Py_readMessage()));


}

void Widget::NEWConnection()
{
    // 클라이언트 연결 대기
   connectedSocket = tcpServer->nextPendingConnection();
    // 데이터 수신
    connect(connectedSocket, SIGNAL(readyRead()), this, SLOT(readMessage()));
    // 연결 종료 수신
    connect(connectedSocket, SIGNAL(disconnected()), SLOT(Client_Disconnected()));
    // 클라이언트와 연결된 소켓에 연결된 클라이언트의 IP 주소
    QString clnt_addr = connectedSocket->peerAddress().toString();
    QString text_conn = QString("<- 클라이언트 연결 성공 (%1)").arg(clnt_addr);
    ui->server_text->append("연결된 소켓 : "+text_conn);

}
//서브 서버 연결
void Widget::PyServer_Connect()
{

    QString serverIP = "10.10.21.110";       //파이썬 서버 ip
    QString serverPort = "31000";           // 파이썬 서버 port
    QHostAddress serverAddress(serverIP);   // 입력된 IP를 프로토콜에 맞는 형태로 가공
    py_tcpSocket->connectToHost(serverAddress, serverPort.toUShort());    //연결: 서버 IP로 연결 요청, Port는 qint16 자료형
    ui->server_text->append("-> 서버에 연결 요청");
    // 연결 완료 메시지를 표시
    if (py_tcpSocket->waitForConnected())
    {
        ui->server_text->append("-> 서버에 연결 완료");
    }
    else
    {
        ui->server_text->append("-> 서버 연결 실패");
    }
}

//클라이언트 메세지 읽기
void Widget::readMessage()
{
    connectedSocket = reinterpret_cast<QTcpSocket*>(sender());
    QByteArray buffer;

    QDataStream socketStream(connectedSocket);
    socketStream.setVersion(QDataStream::Qt_5_15);

    // stream으로 데이터를 읽어들이고, buffer로 넘기면
    socketStream.startTransaction();
    socketStream >> buffer;
    // stream startTransaction 실행 문제시 에러 표시 후 함수 종료
    if(!socketStream.commitTransaction())
    {
        return;
    }

    QString header = buffer.mid(0,128);
    QString fileType = header.split("/!@#/")[0];
    QString fileType_data = header.split("/!@#/")[1];
    ui->server_text->append("-> 데이터 : "+fileType);
    Py_buffer = buffer.mid(128);      // 서브서버로 보낼 버퍼 임시 저장
    buffer = buffer.mid(128);
    QScrollBar *vScrollBar = ui->server_text->verticalScrollBar();
    vScrollBar->setValue(vScrollBar->maximum());
    //음성 파일 데이터
    if(fileType=="attachment")
    {
        cl_mpdata(header);
    }

    //이미지 새로고침
    else if (fileType=="again")
    {
        Server_image();
    }

    // 로그인
    else if (fileType == "login")
    {
        cl_userlogin(header);
    }

    //회원가입(완료)
    else if (fileType == "signup")
    {
        cl_signup(header);
    }
    // 회원가입 (아이디 중복)
    else if (fileType == "checkid")
    {

        cl_signup_id(header);
    }

    //회원가입 (비밀번호 확인)
    else if (fileType == "checkpn")
    {
        cl_signup_pw(header);
    }

    // 회원가입(이메일 중복)
    else if (fileType == "checkmail")
    {
        cl_signup_email(header);
    }

    //아이디찾기
    else if (fileType == "findid")
    {
        cl_findid(header);
    }

    // 비밀번호찾기
    else if (fileType == "findpw")
    {
        cl_findpw(header);
    }
    // 검사용 이미지 전송
    else if (fileType=="resultok")
    {
        cl_resultok(header);
    }
    // 동영상 파일전송
    else if (fileType == "askvideo")
    {
        cl_askvideo(header);
    }
    // 이미지 새로고침
    else if (fileType=="request")
    {

        Server_image();
    }

}

//로그인
void Widget::cl_userlogin(QString header)
{
    QDataStream socketStream(connectedSocket);
    QString  Return_message;
    QSqlQuery query;
    QString id = header.split("/!@#/")[1];
    QString pw = header.split("/!@#/")[2];
    QString pw_ok;
    query.prepare("SELECT PW FROM user WHERE ID = :UserID");
    query.bindValue(":UserID", id);
    ui->server_text->append("-> 로그인 조회 요청");

    if (query.exec() && query.next())
    {
        pw_ok = query.value(0).toString();
        if (pw == pw_ok)
        {
                 ui->server_text->append("로그인 조회 성공");
                Return_message = "loginsuccess/!@#/";
                Connected_List.insert(id, connectedSocket);
                Socket_to_ID.insert(connectedSocket, id);


        }
        else if (pw != pw_ok)
        {
                ui->server_text->append("비밀번호가 일치하지 않습니다.");
                Return_message = "loginfail/!@#/";
        }

    }
    else
    {
        ui->server_text->append("아이디가 일치하지 않습니다.");
        Return_message = "loginfail/!@#/";
    }

    QByteArray headerr;
    headerr.prepend(Return_message.toUtf8());
    headerr.resize(128);
    // stream으로 byteArray 정보 전송
    socketStream << headerr;
    if(pw==pw_ok)
    {
         Server_image();
    }
}

//음성 파일 서브서버로 전송
void Widget::cl_mpdata(QString header)
{
    QByteArray PY_header;
    QString fileName = header.split("/!@#/")[1];
    QString size = header.split("/!@#/")[2];
    QString user_ID = header.split("/!@#/")[3];

    QString filePath = "recordfile/!@#/" + fileName +"/!@#/"+size+"/!@#/"+user_ID+"/!@#/";
    PY_header.prepend(filePath.toUtf8());
    header.resize(128);
    py_tcpSocket->write(PY_header);

}


//회원가입 완료
void Widget:: cl_signup(QString header)
{
    QDataStream socketStream(connectedSocket);
    QString  Return_message;
    QSqlQuery query;
    QString id = header.split("/!@#/")[1];
    QString pw = header.split("/!@#/")[2];
    QString name = header.split("/!@#/")[3];
    QString pn = header.split("/!@#/")[4];
    QString email = header.split("/!@#/")[5];
    int active = 1;

    query.prepare("INSERT INTO user (ID, PW, NAME, PHONE, MAIL, ACTIVE) VALUES (:UserID, :UserPW, :Username, :Userphone, :Useremail, :active)");
    query.bindValue(":UserID", id);
    query.bindValue(":UserPW", pw);
    query.bindValue(":Username", name);
    query.bindValue(":Userphone", pn);
    query.bindValue(":Useremail", email);
    query.bindValue(":active", active);
    query.exec();
    Return_message="signup/!@#/signup_ok/!@#/";
    QByteArray headerr;
    headerr.prepend(Return_message.toUtf8());
    headerr.resize(128);
    // stream으로 byteArray 정보 전송
    socketStream << headerr;
}

// 아이디중복확인 ( 회원가입 )
void Widget:: cl_signup_id(QString header)
{
    QDataStream socketStream(connectedSocket);
    QString  Return_message;
    QSqlQuery query;
    QString id = header.split("/!@#/")[1];

    query.prepare("SELECT COUNT(*) FROM user WHERE ID = :UserID");
    query.bindValue(":UserID", id);

    ui->server_text->append("->아이디 중복확인 요청");
    if (query.exec()&& query.next())
    {

        int idCount = query.value(0).toInt();
        QString id_ck = query.value(0).toString();

        if (idCount == 0)
        {
                ui->server_text->append("중복확인 완료.");
                Return_message="useId_ok/!@#/"+id_ck+"/!@#/";
        }

        else
        {
                 ui->server_text->append("아이디를 찾지 못했습니다.");
                Return_message="useId_no/!@#/";
        }

        QByteArray header;
        header.prepend(Return_message.toUtf8());
        header.resize(128);

        // stream으로 byteArray 정보 전송
        socketStream << header;

    }
}


//서브서버에서 데이터받기
void Widget::Py_readMessage()
{
    if(py_tcpSocket->bytesAvailable() > 0)
    {
        QByteArray readData = py_tcpSocket->read(1024);

        QList<QString>Data_split = QString::fromUtf8(readData).split("/!@#/");
         ui->server_text->append("->서브서버에서 들어온 데이터 = "+Data_split[0]);
        // 파이썬으로 이미지 전송 함수 이동
        if(Data_split[0] == "order")
        {

            PyServer_image();
        }

        //파이썬으로 mp파일 전송 함수 이동
        else if (Data_split[0]=="mp")
        {

            PyData_Return();
        }

        //감정 결과 값 클라이언트로 전송
        else if (Data_split[0]=="result")
        {
            QString PY_user_ID = Data_split[1];
            QString py_message = Data_split[2];

            //결과값 전송
            QTcpSocket* value = Connected_List.value(PY_user_ID);
            QDataStream stream(value);
            stream.setVersion(QDataStream::Qt_5_15);
            QString TEXT_split = "result/!@#/"+py_message+"/!@#/";
            QByteArray header;
            header.prepend(TEXT_split.toUtf8());
            header.resize(128);
            ui->server_text->append("<-결과 값 전송: "+TEXT_split);
            stream << header;

        }


    }
}


    // 비밀번호 중복확인 ( 회원가입 )
void Widget:: cl_signup_pw(QString header)
{
    QDataStream socketStream(connectedSocket);
    QString  Return_message;
    QSqlQuery query;
    QString pw = header.split("/!@#/")[1];

    query.prepare("SELECT COUNT(*) FROM user WHERE PW = :UserPW");
    query.bindValue(":UserPW", pw);
    ui->server_text->append("->비밀번호 중복 조회 요청 "+pw);

    if (query.exec()&& query.next())
    {
            int pwCount = query.value(0).toInt();

            if (pwCount == 0)
            {
                ui->server_text->append("조회 완료 ");
                Return_message="usePN_ok/!@#/";
            }
            else
            {
                ui->server_text->append("비밀번호 중복! ");
                Return_message="usePN_no/!@#/";
            }
            QByteArray header;
            header.prepend(Return_message.toUtf8());
            header.resize(128);
            // stream으로 byteArray 정보 전송
            socketStream << header;
    }
}
// 이메일 중복확인 ( 회원가입 )
void Widget:: cl_signup_email(QString header)
{
    QDataStream socketStream(connectedSocket);
    QString  Return_message;
    QSqlQuery query;
    QString mail = header.split("/!@#/")[1];

    query.prepare("SELECT COUNT(*) FROM user WHERE MAIL = :mail");
    query.bindValue(":mail", mail);

ui->server_text->append("->이메일 중복 조회 요청");
    if (query.exec()&& query.next())
    {
            int mailCount = query.value(0).toInt();
            if (mailCount == 0)
            {
                ui->server_text->append("조회 완료");
                Return_message="useEmail_ok/!@#/";
            }
            else
            {
                ui->server_text->append("이메일 중복!");
                Return_message="useEmail_no/!@#/";
            }
            QByteArray header;
            header.prepend(Return_message.toUtf8());
            header.resize(128);
            // stream으로 byteArray 정보 전송
            socketStream << header;
    }
}
// 아이디 찾기
void Widget:: cl_findid(QString header)
{
    QDataStream socketStream(connectedSocket);
    QString Return_message;
    QSqlQuery query;
    QString findid_name = header.split("/!@#/")[1];
    QString findid_email = header.split("/!@#/")[2];

    query.prepare("SELECT ID FROM user WHERE NAME = :Username AND MAIL = :Useremail");
    query.bindValue(":Username", findid_name);
    query.bindValue(":Useremail", findid_email);
    ui->server_text->append("->아이디찾기 조회 요청");
    if (query.exec() && query.next())
    {
        QString ID_ck = query.value(0).toString();
        ui->server_text->append("아이디찾기 조회 :" + ID_ck);
        Return_message = "matchingID_ok/!@#/" + ID_ck+"/!@#/";
    }
    else
    {
         ui->server_text->append("조회된 아이디가 없습니다..");
        Return_message = "matchingID_no/!@#/";
    }



    QByteArray headerr;
    headerr.prepend(Return_message.toUtf8());
    headerr.resize(128);
    socketStream << headerr;
}

//비밀번호 찾기
void Widget:: cl_findpw(QString header)
{
    QDataStream socketStream(connectedSocket);
    QString Return_message;
    QSqlQuery query;
    QString findid_ID = header.split("/!@#/")[1];
    QString findid_PN = header.split("/!@#/")[2];

    query.prepare("SELECT PW FROM user WHERE ID = :UserID AND PHONE = :UserPN");
    query.bindValue(":UserID", findid_ID);
    query.bindValue(":UserPN", findid_PN);
    ui->server_text->append("->비밀번호찾기 조회 요청");
    if (query.exec()&& query.next())
    {
        QString PW_ck = query.value(0).toString();
        ui->server_text->append("조회 완료");
        Return_message="matchingPW_ok/!@#/"+PW_ck+"/!@#/";
    }

    else
    {
        ui->server_text->append("조회된 비밀번호가 없습니다");
        Return_message="matchingPW_no/!@#/";
    }
    QByteArray headerr;
    headerr.prepend(Return_message.toUtf8());
    headerr.resize(128);
    socketStream << headerr;
}

//클라이언트 썸네일 전송
void Widget:: cl_resultok(QString header)
{
    QDataStream socketStream(connectedSocket);
    QString user_ID = header.split("/!@#/")[1];
    QString message = header.split("/!@#/")[2];
    QString em_splits="/!@#/";
    QSqlQuery query;
    QVector <QString>VIDEO_IMAGE;

   ui->server_text->append(" 클라이언트로 썸네일 전송 중...");
    QScrollBar *vScrollBar = ui->server_text->verticalScrollBar();
    vScrollBar->setValue(vScrollBar->maximum());
    query.prepare("SELECT testimage FROM emlist WHERE EMOTION = :EMOTIONN");
    query.bindValue(":EMOTIONN", message);

    if(query.exec())
    {
            while (query.next())
            {
                VIDEO_IMAGE.append(query.value(0).toString());
            }

    }
    for(int i =0; i<VIDEO_IMAGE.size(); i++)
    {
            QFile imageFile(VIDEO_IMAGE[i]);
            QByteArray em_headedr;
            //이미지 전송
            if (imageFile.open(QIODevice::ReadOnly))
            {
                em_headedr=imageFile.readAll();
                socketStream << em_headedr;

            }
    }
    ui->server_text->append("<-전송 완료!");


}

//영상 전송
void Widget:: cl_askvideo(QString header)
{
    QDataStream socketStream(connectedSocket);
    QString message = header.split("/!@#/")[2];
    QVector <QString>URL;
    QSqlQuery query;
    QByteArray url_header;
    QString em_splits="/!@#/";
    QString em_split="em_video/!@#/";

    query.prepare("SELECT URL FROM emlist WHERE EMOTION = :EMOTIONN");
    query.bindValue(":EMOTIONN", message);
    ui->server_text->append("<-클라이언트로 영상 전송 중 ...");
    if(query.exec())
    {
            while (query.next())
            {
                URL.append(query.value(0).toString());

            }
    }
    url_header.prepend(em_split.toUtf8());
    url_header.resize(128);
    for(int i = 0 ; i<URL.size(); i++)
    {
            url_header.append(URL[i].toUtf8());
            url_header.append(em_splits.toUtf8());
    }

    socketStream << url_header;
    ui->server_text->append("<-전송 완료!");
}





//서버 -> 파이썬서버 데이터 전송
void Widget::PyData_Return()
{
    QByteArray mp_data;
    int bytesRead = 0;
    ui->server_text->append("<-서브서버로 음성데이터 전송 중..");
    // 데이터를 계속 읽어서 전송
    while (bytesRead < Py_buffer.size())
    {
        mp_data = Py_buffer.mid(bytesRead,1024);
        qint64 bytesWritten = py_tcpSocket->write(mp_data);
        bytesRead += mp_data.size();

    }
    ui->server_text->append("<-전송 완료!");
}



//클라이언트 이미지 전송
void Widget::Server_image()
{
    QString connect_id;
    Random_image_idx = QRandomGenerator::global()->bounded(Image_Vector.size()); //이미지 인덱스를 기준으로 랜덤값 생성
    Rand_FilePath = Image_Vector[Random_image_idx]; // 랜덤으로 고른 이미지 경로 저장
    QStringList rand_splitt = Rand_FilePath.split("/");
    QString lastPart = rand_splitt.last();

    QFileInfo fileInfo(Rand_FilePath);
    qint64 imageSize = fileInfo.size(); // 이미지 파일 크기 (바이트 단위)

    if (Socket_to_ID.contains(connectedSocket))
    {

        connect_id = Socket_to_ID.value(connectedSocket);
    }


    // 파일전송 구분자

    QString filePath = "fileinfo/!@#/"+ lastPart +"/!@#/" +QString::number(imageSize) +"/!@#/"+connect_id;


    QByteArray py_header=filePath.toUtf8();

    py_tcpSocket->write(py_header); // 서브서버로 구분자 전송 (전송 시간에 텀을주기위해 )

    QByteArray header;
    header.prepend(filePath.toUtf8());
    header.resize(128);

    QDataStream subServerStream(connectedSocket);//클라이언트 연결
    subServerStream.setVersion(QDataStream::Qt_5_15);
    subServerStream.startTransaction();


    QFile imageFile(Rand_FilePath);
    if (imageFile.open(QIODevice::ReadOnly))
    {
        QByteArray byteArray = imageFile.readAll();
        byteArray.prepend(header);
        subServerStream << byteArray;
        imageFile.close();
        ui->server_text->append("<-클라이언트 감정 테스트 이미지 전송 완료!");
        QScrollBar *vScrollBar = ui->server_text->verticalScrollBar();
        vScrollBar->setValue(vScrollBar->maximum());
    }
    else
    {
        ui->server_text->append("이미지 파일을 열 수 없음");
    }

    if (!subServerStream.commitTransaction())
    {
        ui->server_text->append("전송 실패");
    }
}


//서브서버 이미지 보내기
void Widget::PyServer_image()
{

    QByteArray fileData;
    QFile imageFile(Rand_FilePath);
    if (imageFile.open(QIODevice::ReadOnly))
     {
        while(!imageFile.atEnd())
        {
            fileData = imageFile.read(1024);
            py_tcpSocket->write(fileData);
            py_tcpSocket->waitForBytesWritten();
        }
         ui->server_text->append("<-서브서버 감정 테스트 이미지 전송 완료!");
    }
    else
    {
        ui->server_text->append("이미지 파일을 열 수 없음");
    }
}



//연결 종료
void Widget::Client_Disconnected()
{
    QTcpSocket *socket = static_cast<QTcpSocket*> (sender());
    QString clnt_addr = socket -> peerAddress().toString();
    Connected_List.remove(clnt_addr);
    QString text_close =clnt_addr+"-> 클라이언트 접속 종료";
    ui -> server_text -> append(text_close);
}



Widget::~Widget()
{
    delete ui;
}



