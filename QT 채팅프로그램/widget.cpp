#include "widget.h"
#include "ui_widget.h"

Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{
    ui->setupUi(this);
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName("/home/lms110/문서/7team/chat.db");
    db.open();

    initialize();
}

void Widget::initialize()
{
    QHostAddress hostAddress;
    QList<QHostAddress> ipAddressesList = QNetworkInterface::allAddresses();

    for (int i = 0; i < ipAddressesList.size(); ++i)
    {
        if (ipAddressesList.at(i) != QHostAddress::LocalHost && ipAddressesList.at(i).toIPv4Address())
        {
            hostAddress = ipAddressesList.at(i);
            break;
        }
    }

    if (hostAddress.toString().isEmpty())
    {
        hostAddress = QHostAddress(QHostAddress::LocalHost);
    }

    kakaoServer = new QTcpServer(this);

    if (!kakaoServer -> listen(hostAddress, 25000))
    {
        QMessageBox::critical(this, tr("TCP Server"),
                              tr("서버를 시작할 수 없습니다. 에러메세지 : %1.")
                              .arg(kakaoServer->errorString()));
        close();
    }

    connect(kakaoServer, SIGNAL(newConnection()), this, SLOT(newConnection2()));
}

void Widget::newConnection2()
{
    QTcpSocket* new_socket = kakaoServer -> nextPendingConnection();
    serverSocket_list.append(new_socket);
    qDebug() << "소켓리스트:" << serverSocket_list;

    connect(new_socket, SIGNAL(disconnected()), this, SLOT(disconnected_func()));
    connect(new_socket, SIGNAL(readyRead()), this, SLOT(readMessage()));

    ui -> connectlog -> append("클라이언트 연결 완료");
}

void Widget::readMessage()
{
    QSqlQuery query;

    QTcpSocket *new_socket = static_cast<QTcpSocket*>(sender());

    QByteArray readData = new_socket -> readAll();

    QString message = QString::fromUtf8(readData);
    clnt = message.split("/$+^7/");
    QString clnt_adrr = new_socket -> peerAddress().toString();
    ui -> chatlog -> append(message);


    qDebug() << "받은 메시지 : " << message;

    //채팅 clnt[1]은 방번호로 지정하는게 좋을 것 같습니다.
    if (clnt[0] == "chat" && clnt[1] == "")
    {
        QString chat_msg = "chat/$+^7/" + clnt[2];
        QByteArray messageData = chat_msg.toUtf8();
        for (QTcpSocket *socket : serverSocket_list)
        {
            if (socket -> state() == QAbstractSocket::ConnectedState && socket != new_socket)
            {
                socket -> write(messageData);
            }
        }
    }

    //로그인
    else if (clnt[0] == "Login" && clnt[1] == "in")
    {
        ui -> newmember_Edit -> append(clnt[2]);
        ui -> newmember_Edit -> append(clnt[3]);
        query.prepare("SELECT PW, NICKNAME FROM members WHERE ID = :value");
        query.bindValue(":value", clnt[2]);

        if (query.exec() && query.next())
        {
            QString logPW = query.value(0).toString();
            QString logNick = query.value(1).toString();

            if (logPW == clnt[3])
            {
                query.prepare("SELECT ACTIVE FROM members WHERE ID =:ID");
                query.bindValue(":ID", clnt[2]);
                if (query.exec() && query.next())
                {
                    QString active1 = query.value(0).toString();
                    if (active1 == "0")
                    {
                        query.prepare("UPDATE members SET ACTIVE = '1' WHERE ID = :ID AND PW = :PW");
                        query.bindValue(":ID", clnt[2]);
                        query.bindValue(":PW", clnt[3]);
                        if (query.exec())
                        {
                            QString ok = "Login/$+^7/success/$+^7/"+logNick;
                            QByteArray okay = ok.toUtf8();
                            new_socket -> write(okay);
                            select_socket[clnt[2]] = new_socket;
                            qDebug() << select_socket;
                        }
                    }
                    else if (active1 == "1")
                    {
                        QString whoareyou = "Login/$+^7/whowho/$+^7/nope";
                        QByteArray whoareU = whoareyou.toUtf8();
                        new_socket -> write(whoareU);
                    }
                }
            }
            else
            {
                QString no = "Login/$+^7/fail";
                QByteArray nono = no.toUtf8();
                new_socket -> write(nono);
                qDebug() << "로그인 실패";
            }
        }
        else
        {
            QString who = "Login/$+^7/who";
            QByteArray whowho = who.toUtf8();
            new_socket -> write(whowho);
            qDebug() << "회원 정보 없음";
        }
    }


    //회원가입
    else if (clnt[0] == "newID" && clnt[1] == "auto")
    {
        query.prepare("SELECT ID FROM members WHERE ID = :value");
        query.bindValue(":value", clnt[2]);
        if (query.exec())
        {
            if (query.next())
            {
                QString new_id = "new_checkID/$+^7/auto/$+^7/NO";
                QByteArray new_new = new_id.toUtf8();
                new_socket -> write(new_new);
            }
            else
            {
                QString new_id = "new_checkID/$+^7/auto/$+^7/YES";
                QByteArray new_new = new_id.toUtf8();
                new_socket -> write(new_new);
            }
        }
    }
    else if (clnt[0] == "newNick" && clnt[1] == "auto")
    {
        query.prepare("SELECT NICKNAME FROM members WHERE NICKNAME = :value");
        query.bindValue(":value", clnt[2]);
        if (query.exec())
        {
            if (query.next())
            {
                QString new_id = "new_checkNick/$+^7/auto/$+^7/NO";
                QByteArray new_new = new_id.toUtf8();
                new_socket -> write(new_new);
            }
            else
            {
                QString new_id = "new_checkNick/$+^7/auto/$+^7/YES";
                QByteArray new_new = new_id.toUtf8();
                new_socket -> write(new_new);
            }
        }
    }
    else if (clnt[0] == "newPhone" && clnt[1] == "auto")
    {
        query.prepare("SELECT PHONE FROM members WHERE PHONE = :value");
        query.bindValue(":value", clnt[2]);
        if (query.exec())
        {
            if (query.next())
            {
                QString new_id = "new_checkPhone/$+^7/auto/$+^7/NO";
                QByteArray new_new = new_id.toUtf8();
                new_socket -> write(new_new);
            }
            else
            {
                QString new_id = "new_checkPhone/$+^7/auto/$+^7/YES";
                QByteArray new_new = new_id.toUtf8();
                new_socket -> write(new_new);
            }
        }
    }
    else if (clnt[0] == "newEmail" && clnt[1] == "auto")
    {
        query.prepare("SELECT EMAIL FROM members WHERE EMAIL = :value");
        query.bindValue(":value", clnt[2]);
        if (query.exec())
        {
            if (query.next())
            {
                QString new_id = "new_checkEmail/$+^7/auto/$+^7/NO";
                QByteArray new_new = new_id.toUtf8();
                new_socket -> write(new_new);
            }
            else
            {
                QString new_id = "new_checkEmail/$+^7/auto/$+^7/YES";
                QByteArray new_new = new_id.toUtf8();
                new_socket -> write(new_new);
            }
        }
    }
    else if (clnt[0] == "Hello" && clnt[1] == "newmember")
    {
        query.prepare("INSERT INTO members (ID, PW, NAME, NICKNAME, PHONE, EMAIL, ACTIVE) VALUES (:ID, :PW, :NAME, :NICKNAME, :PHONE, :EMAIL, :ACTIVE)");
        query.bindValue(":ID", clnt[2]);
        query.bindValue(":PW", clnt[3]);
        query.bindValue(":NAME", clnt[4]);
        query.bindValue(":NICKNAME", clnt[5]);
        query.bindValue(":PHONE", clnt[6]);
        query.bindValue(":EMAIL", clnt[7]);
        query.bindValue(":ACTIVE", "0");
        query.exec();

        QString yes = "Hello/$+^7/newmember";
        QByteArray yes_msg = yes.toUtf8();
        new_socket -> write(yes_msg);
    }

    //아이디 찾기
    else if (clnt[0] == "Find" && clnt[1] == "ID")
    {
        query.prepare("SELECT ID FROM members WHERE NAME = :value1 AND PHONE = :value2 AND EMAIL = :value3");
        query.bindValue(":value1", clnt[2]);
        query.bindValue(":value2", clnt[3]);
        query.bindValue(":value3", clnt[4]);
        qDebug() << clnt;

        if (query.exec() && query.next())
        {
            QString found_ID = "Found/$+^7/ID/$+^7/YES/$+^7/" + query.value(0).toString();
            QByteArray ok_found_ID = found_ID.toUtf8();
            new_socket -> write(ok_found_ID);
        }
        else
        {
            QString Not_found = "Found/$+^7/ID/$+^7/NOT";
            QByteArray Not_found_ID = Not_found.toUtf8();
            new_socket -> write(Not_found_ID);
        }
    }

    //비밀번호 재설정
    else if (clnt[0] == "SetPW" && clnt[1] == "PW")
    {
        query.prepare("SELECT PHONE, EMAIL FROM members WHERE ID = :ID");
        query.bindValue(":ID", clnt[2]);
        qDebug() << clnt[2];
        if (query.exec() && query.next())
        {
            qDebug() << "들어왔나";
            QString logPhone = query.value(0).toString();
            QString logEmail = query.value(1).toString();
            qDebug() << logPhone;
            if (logPhone == clnt[3] && logEmail == clnt[4])
            {
                query.prepare("UPDATE members SET PW = :value WHERE ID = :ID AND PHONE = :PHONE AND EMAIL = :EMAIL");
                query.bindValue(":value", clnt[5]);
                query.bindValue(":ID", clnt[2]);
                query.bindValue(":PHONE", clnt[3]);
                query.bindValue(":EMAIL", clnt[4]);

                if (query.exec())
                {
                    QString set_PW12 = "Settin/$+^7/setting/$+^7/success";
                    QByteArray setting_PW = set_PW12.toUtf8();
                    new_socket -> write(setting_PW);
                    ui -> connectlog -> append(setting_PW);
                }
            }
            else
            {
                QString set_PW123 = "Settin/$+^7/setting/$+^7/fail";
                QByteArray setting_PW123 = set_PW123.toUtf8();
                new_socket -> write(setting_PW123);
                ui -> connectlog -> append(setting_PW123);
            }
        }
        else {
            QString set_PW123 = "Settin/$+^7/setting/$+^7/fail";
            QByteArray setting_PW123 = set_PW123.toUtf8();
            new_socket -> write(setting_PW123);
            ui -> connectlog -> append(setting_PW123);
        }
    }

    //잠금모드
    else if (clnt[0] == "Mode" && clnt[1] == "block")
    {
        query.prepare("SELECT ACTIVE FROM members WHERE ID= :id AND PW = :pw");
        query.bindValue(":id", clnt[2]);
        query.bindValue(":pw", clnt[3]);
        qDebug() << clnt[2] << clnt[3];

        if (query.exec() && query.next())
        {
            QString act = query.value(0).toString();
            if (act == "3")
            {
                QString throwAct1 = "Mode/$+^7/block/$+^7/okay";
                QByteArray throwAct2 = throwAct1.toUtf8();
                new_socket -> write(throwAct2);
            }
            else
            {
                QString throwAct3 = "Mode/$+^7/block/$+^7/not";
                QByteArray throwAct4 = throwAct3.toUtf8();
                new_socket -> write(throwAct4);
            }
        }
        else
        {
            QString throwNot1 = "Mode/$+^7/block/$+^7/blocking";
            QByteArray throwNot2 = throwNot1.toUtf8();
            new_socket -> write(throwNot2);
        }
    }
    else if (clnt[0] == "Block" && clnt[1] == "mode")
    {
        query.prepare("UPDATE members SET ACTIVE = '3' WHERE NICKNAME = :nick");
        query.bindValue(":nick", clnt[2]);

        if (query.exec())
        {
            QString blockokay1 = "Block/$+^7/okay";
            QByteArray blockokay2 = blockokay1.toUtf8();
            new_socket -> write(blockokay2);
            qDebug() << blockokay1 << ":" << blockokay2;
        }
    }

    //닉네임 중복 검사
    else if (clnt[0] == "Nick" && clnt[1] == "autochange")
    {
        query.prepare("SELECT NICKNAME FROM members WHERE NICKNAME = :value");
        query.bindValue(":value", clnt[2]);
        if (query.exec())
        {
            if (query.next())
            {
                QString new_id = "ChangeNick/$+^7/auto/$+^7/NO";
                QByteArray new_new = new_id.toUtf8();
                new_socket -> write(new_new);
            }
            else
            {
                QString new_id = "ChangeNick/$+^7/auto/$+^7/YES";
                QByteArray new_new = new_id.toUtf8();
                new_socket -> write(new_new);
            }
        }
    }

    //닉네임 변경하기
    else if (clnt[0] == "Nick" && clnt[1] == "change")
    {
        query.prepare("SELECT PW FROM members WHERE ID = :id");
        query.bindValue(":id", clnt[2]);
        if (query.exec() && query.next())
        {
            QString PW_info = query.value(0).toString();
            if (PW_info == clnt[3])
            {
                query.prepare("UPDATE members SET NICKNAME = :nickname WHERE ID = :id");
                query.bindValue(":id", clnt[2]);
                query.bindValue(":nickname", clnt[4]);
                query.exec();
                QString chg = "Change/$+^7/YOUR/$+^7/NICKNAME/$+^7/"+clnt[4];
                QByteArray chgnick = chg.toUtf8();
                qDebug() << clnt[4];
                new_socket -> write(chgnick);
            }
            else
            {
                QString chg1 = "NOT/$+^7/Change/$+^7/YOURNICK";
                QByteArray chg2 = chg1.toUtf8();
                new_socket -> write(chg2);
            }
        }
    }

    //내정보
    else if (clnt[0] == "Info" && clnt[1] == "my_info")
    {
        query.prepare("SELECT ID, NAME, NICKNAME, PHONE, EMAIL FROM members WHERE NICKNAME = :nick");
        query.bindValue(":nick", clnt[2]);
        if (query.exec() && query.next())
        {
            QString ID1 = query.value(0).toString();
            QString NAME1 = query.value(1).toString();
            QString NICKNAME1 = query.value(2).toString();
            QString PHONE1 = query.value(3).toString();
            QString EMAIL1 = query.value(4).toString();

            QString all_info1 = "YOUR/$+^7/info/$+^7/" + ID1 + "/$+^7/" + NAME1 + "/$+^7/" + NICKNAME1 + "/$+^7/" + PHONE1 + "/$+^7/" + EMAIL1;
            QByteArray all_info2 = all_info1.toUtf8();
            new_socket -> write(all_info2);
            qDebug() << "내정보 보냇슈";
        }
    }

    //로그아웃
    else if (clnt[0] == "log" && clnt[1] == "out")
    {

        query.prepare("UPDATE members SET ACTIVE = '0' WHERE ID = :ID");

        query.bindValue(":ID", clnt[2]);

        if (query.exec() && query.next())
        {
            qDebug() << "완료" << select_socket;
            int indexToRemove = serverSocket_list.indexOf(new_socket);
            if (indexToRemove != -1)
            {
                QTcpSocket* socketToRemove = serverSocket_list.takeAt(indexToRemove);
                socketToRemove->close();
                delete socketToRemove;
                qDebug() << "소켓리스트:" << serverSocket_list;
            }
        }
    }

    // 메시지 뿌리기
    else if (clnt[0] == "single" || clnt[0] == "team") {
        qDebug() << "뭐받았니" << clnt[0] << clnt[1] << clnt[2];
        QVector<QTcpSocket*> send_socket;
        QByteArray change_msg;


       //clnt[0] 타입 clnt [1] 방번호 clnt[2] 닉네임 clnt[3] 할말
        QString find_invite_id;
        QString orginal_mem;
        QStringList message_split = clnt[3].split(" ");
        qDebug() << "1" << message_split;
        if(message_split[0] == "@초대")
        {
            query.prepare("SELECT * FROM members WHERE NICKNAME= :nick");
            query.bindValue(":nick", message_split[1]);
            if(query.exec() && query.next()) {
                find_invite_id = query.value("ID").toString();
            }
            qDebug() << "2" <<find_invite_id << clnt[3];

            if(find_invite_id == "") {

                QString chat_msg = "invite/$+^7/fail/$+^7/유저 정보가 없습니다.";
                QByteArray messageData = chat_msg.toUtf8();
                QTcpSocket *socket = new_socket;
                new_socket -> write(messageData);
            }

            else {
                query.prepare("SELECT * FROM chating WHERE NUM =:num");
                query.bindValue(":num", clnt[1]);
                if (query.exec() && query.next()) {
                    orginal_mem = query.value("MEMBER").toString();
                }
                QString update_member = orginal_mem+ "," +find_invite_id;
                qDebug() << "3" <<orginal_mem << update_member;

                query.prepare("UPDATE chating SET TYPE = :type, MEMBER= :member where NUM= :num");
                query.bindValue(":type", "team");
                query.bindValue(":member", update_member);
                query.bindValue(":num", clnt[1]);
                query.exec();

                QString chat_msg = "invite/$+^7/okay/$+^7/대화방에 초대되었습니다.";
                QByteArray messageData = chat_msg.toUtf8();
                QTcpSocket* key_value = select_socket.value(find_invite_id);
                key_value -> write(messageData);
            }
        }

        else if (message_split[0] == "@호출") {
            QString find_call_id;
            query.prepare("SELECT * FROM members WHERE NICKNAME =:nick");
            query.bindValue(":nick", message_split[1]);
            if(query.exec() && query.next()) {
                find_call_id = query.value("ID").toString();
            }
            if(find_call_id == "") {

            }
            else {
                QString chat_msg = "call/$+^7/user/$+^7/"+clnt[2]+":/$+^7/"+message_split[2];
                QByteArray messageData = chat_msg.toUtf8();
                QTcpSocket* key_value = select_socket.value(find_call_id);
                key_value -> write(messageData);
            }
        }


        else
        {
            QString send_msg = "chat/$+^7/"+clnt[1]+ "/$+^7/" + clnt[2] + "/$+^7/" + clnt[3];
            change_msg = send_msg.toUtf8();

            query.prepare("SELECT * FROM chating WHERE NUM= :num");
            query.bindValue(":num", clnt[1]);
            if(query.exec() && query.next()) {
                QString rec_member = query.value("MEMBER").toString();
                QStringList memberList = rec_member.split(',');

                for(int idx = 0; idx < memberList.size(); idx++) {
                    QTcpSocket* key_value = select_socket.value(memberList[idx]);
                    if(memberList[idx] != nullptr) {
                        send_socket.append(key_value);
                    }
                }
            }
            for(QTcpSocket* socket: send_socket) {
                if(socket->state() == QAbstractSocket::ConnectedState && socket != new_socket)
                {
                    socket -> write(change_msg);
                }
            }
        }
    }

    else if (clnt[0] == "room" && clnt[1] == "lock")
    {


        query.prepare("UPDATE chating SET LOCK = :newLockValue WHERE NUM = :num2");
        query.bindValue(":newLockValue", clnt[2]);
        query.bindValue(":num2",clnt[3]);
        qDebug()<<clnt[3]<< "넘버들어옴?";
        if (query.exec())
        {
            query.prepare("SELECT LOCK FROM chating WHERE NUM = :num2 ");
            query.bindValue(":num2", clnt[3]);
            if (query.exec() && query.next())
            {
                QString LOCK_RETURN = query.value(0).toString();
                QString ROOM_LOCK_return = "ROOM/$+^7/LOCK/$+^7/"+LOCK_RETURN+"/$+^7/"+clnt[3];

                QByteArray ROOM_LOCK_return2 = ROOM_LOCK_return.toUtf8();

                for (QTcpSocket *socket : serverSocket_list)
                {
                    if (socket->state() == QAbstractSocket::ConnectedState)
                    {
                        socket->write(ROOM_LOCK_return2);
                    }
                }
            }

        }
    }

    //상길
    else if (clnt[0] == "user" && clnt[1] == "chodae")
    {
        QString room_num_str = QString::number(room_num);
        QString front = clnt[2]; // 초대보낼 사람 닉네임 가져옴
        QString back = clnt[3]; // 보낸 사람 닉네임 가져옴
        qDebug() << "방번호:" << room_num;
        QString TYPE2 = "single";

        // front의 ID 조회
        query.prepare("SELECT ID FROM members WHERE NICKNAME = :NICKNAME2 ");
        query.bindValue(":NICKNAME2", front);

        if (query.exec() && query.next())
        {
            QString user_ID = query.value(0).toString();   // 찾을 사람 아이디 저장

            // 먼저 사용자의 소켓을 select_socket 맵에서 찾습니다.
            QTcpSocket* useer = select_socket.value(user_ID);

            if (useer)
            {
                // back의 ID 조회
                query.prepare("SELECT ID FROM members WHERE NICKNAME = :NICKNAME3");
                query.bindValue(":NICKNAME3", back);  // 디비 저장용 닉네임

                if (query.exec() && query.next())
                {
                    QString id = query.value(0).toString();

                    // 두 번째 사용자의 ID를 조회
                    query.prepare("SELECT ID FROM members WHERE NICKNAME = :NICKNAME4");
                    query.bindValue(":NICKNAME4", front);  // 디비 저장용 닉네임

                    if (query.exec() && query.next())
                    {
                        QString id2 = query.value(0).toString();
                        QString user_member = id + "," + id2;

                        query.prepare("INSERT INTO chating (TYPE , NUM , MEMBER, LOCK) VALUES (:TYPE, :NUM, :MEMBER, :LOCK)");
                        query.bindValue(":TYPE", TYPE2);
                        query.bindValue(":NUM", room_num_str);
                        query.bindValue(":MEMBER", user_member);
                        query.bindValue(":LOCK", "0");

                        if (query.exec())
                        {
                            QString user_chodae = "user/$+^7/cho_dae";
                            QByteArray user_chodae_bytes = user_chodae.toUtf8();
                            qDebug() << user_chodae_bytes << "확인하셈";
                            useer->write(user_chodae_bytes);
                            new_socket->write(user_chodae_bytes);
                            room_num++;
                            qDebug()<<room_num<<"dsa";
                        }

                    }

                }

            }

        }
    }




    //상길 채팅방참여자 이름, 번호, 채팅방번
    else if (clnt[0] == "room" && clnt[1] == "in")
    {
        QString temp_id = clnt[2];
        QVector<QString> type2;
        QVector<QString> member2;
        QVector<QString> num2;
        QVector<QString> memo2;
        qDebug()<<clnt[2]<<"dㅇㅇㅇㅇsa";

        query.exec("SELECT * FROM chating WHERE member LIKE '%" + temp_id + "%';");
        while (query.next()){
            QString type = query.value("TYPE").toString(); // TYPE 열의 값을 문자열로 가져오기
            QString member = query.value("MEMBER").toString();
            QString num = query.value("NUM").toString();
            type2.append(type);
            member2.append(member);
            num2.append(num);
        }
        //
        QVector<QString> nick2;
        QVector<QString> active2;
        query.exec("SELECT * FROM members");
        while (query.next()) {
            QString nick = query.value("NICKNAME").toString();
            QString active = query.value("ACTIVE").toString();
            QString memo = query.value("STATUS").toString();
            nick2.append(nick);
            active2.append(active);
            memo2.append(memo);
        }

        //

        QString RANDOM = "SSS";
        QByteArray byteArray;
        QDataStream stream(&byteArray, QIODevice::WriteOnly);
        stream << RANDOM<<type2<<member2<<num2<<nick2<<active2<<memo2;
        new_socket->write(byteArray);

    }
}

void Widget::on_serverchatButton_clicked()
{
    QByteArray msg_text = ui -> serverchatline -> text().toUtf8();
    QByteArray message = QByteArray(msg_text);


    for (QTcpSocket *socket : serverSocket_list)
    {
        if (socket -> state() == QAbstractSocket::ConnectedState)
        {
            socket -> write(message);
        }
    }
    QString log = ui -> serverchatline -> text();
    ui -> chatlog -> append(log);
    ui -> serverchatline -> clear();
}

Widget::~Widget()
{
    delete ui;
}

// 7 함수 추가
void Widget::disconnected_func() {
    QTcpSocket *new_socket = static_cast<QTcpSocket*>(sender());
    serverSocket_list.removeOne(new_socket);
    QString text_close = QString("->클라이언트 접속 종료");
    ui -> chatlog -> append(text_close);
}


