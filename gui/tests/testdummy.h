#ifndef TESTDUMMY_H
#define TESTDUMMY_H

#include <QtTest>

class TestDummy : public QObject
{
    Q_OBJECT

private slots:
    void testDummy();
};

#endif // TESTDUMMY_H
