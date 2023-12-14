#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QtNetwork>
#include <QTcpSocket>
#include <QFileDialog>
#include <QMessageBox>
#include <QRandomGenerator>
#include <QSqlDatabase>
#include <QtSql>
#include <QIODevice>
#include <QScrollBar>
QT_BEGIN_NAMESPACE
namespace Ui { class Widget; }
QT_END_NAMESPACE

class Widget : public QWidget
{
    Q_OBJECT

public:
    Widget(QWidget *parent = nullptr);
    ~Widget();

private:
    Ui::Widget *ui;

    void initialize();
    void PyData_Return( );              // 파이썬으로 데이터 전송
    void Server_image();                // 서버 이미지 전송
    void PyServer_image();              // 파이썬 서버 이미지 전송
    void PyServer_Connect();            // 파이썬서버 연결
    void cl_mpdata(QString header);     //음성데이터전송
    void cl_userlogin(QString header ); //로그인
    void cl_signup(QString header);     //회원가입
    void cl_signup_id(QString header);  // 아이디 중복확인
    void cl_signup_pw(QString header);  // 비밀번호 중복확인
    void cl_signup_email(QString header); // 이메일 중복확인
    void cl_findid(QString header);       // 아이디 찾기
    void cl_findpw(QString header);       //비밀번호 찾기
    void cl_resultok(QString header);     // 이미지 전송
    void cl_askvideo(QString header);     // 동영상 전송
    void Py_Result(QString PY_user_ID,QString py_message);
    
    QSqlDatabase db;                               //db 연결
    QTcpServer *tcpServer;                         //서버(클라이언트와 연결)
    QMap<QString,QTcpSocket*> Connected_List;      //소켓 리스트
    QMap<QTcpSocket*, QString> Socket_to_ID;       //아이디소켓
    QTcpSocket*connectedSocket;                    //연결된 소켓
    QVector<QString> Image_Vector;                 //이미지 저장 백터
    int Random_image_idx;                          // 이미지랜덤으로 뽑아오기
    // 서브 서버
    QTcpSocket *py_tcpSocket;                      //클라이언트(서브서버와 연결)
    QString Rand_FilePath;                         //파일경로 저장
    QByteArray Py_buffer;                         // 파이썬 서버로 보낼 버퍼 임시

private slots:
    void NEWConnection();                          // 클라이언트 연결
    void readMessage();                            //클라이언트 메세지
    void Py_readMessage();                         //파이썬 메세지
    void Client_Disconnected();                    // 클라이언트 연결해제


};
#endif // WIDGET_H
