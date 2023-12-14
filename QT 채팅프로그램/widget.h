#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QtNetwork>
#include <QtSql>
#include <QFileInfo>
#include <QMessageBox>
#include <QtDebug>
#include <QVector>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>

class QTcpServer;
class QNetWorkSession;

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
    int room_num = 5;
    QTcpSocket *connectedSocket;
    QTcpServer *kakaoServer;
    QList<QTcpSocket*> serverSocket_list;
    QVector<QString> clnt;
    QMap<QString, QTcpSocket*> select_socket; //3 소켓저장
    QString ClientID; //상길
    QString user_chat; //상길
    // 로그아웃 기능 구현 시, 맵에서 삭제
    // select_socket.remove(logout_id);
private slots:
    void newConnection2();
    void readMessage();
    void on_serverchatButton_clicked();
    void disconnected_func(); //4 서버 종료 함수
};
#endif // WIDGET_H
