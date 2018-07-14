// Copyright (c) 2009-2014 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "uritests.h"

#include "guiutil.h"
#include "walletmodel.h"

#include <QUrl>

void URITests::uriTests()
{
    SendCoinsRecipient rv;
    QUrl uri;
    uri.setUrl(QString("massgrid:mfb4XJGyaBwNK2Lf4a7r643U3JotRYNw2T?req-dontexist="));
    QVERIFY(!GUIUtil::parseMassGridURI(uri, &rv));

    uri.setUrl(QString("massgrid:mfb4XJGyaBwNK2Lf4a7r643U3JotRYNw2T?dontexist="));
    QVERIFY(GUIUtil::parseMassGridURI(uri, &rv));
    QVERIFY(rv.address == QString("mfb4XJGyaBwNK2Lf4a7r643U3JotRYNw2T"));
    QVERIFY(rv.label == QString());
    QVERIFY(rv.amount == 0);

    uri.setUrl(QString("massgrid:mfb4XJGyaBwNK2Lf4a7r643U3JotRYNw2T?label=Some Example Address"));
    QVERIFY(GUIUtil::parseMassGridURI(uri, &rv));
    QVERIFY(rv.address == QString("mfb4XJGyaBwNK2Lf4a7r643U3JotRYNw2T"));
    QVERIFY(rv.label == QString("Some Example Address"));
    QVERIFY(rv.amount == 0);

    uri.setUrl(QString("massgrid:mfb4XJGyaBwNK2Lf4a7r643U3JotRYNw2T?amount=0.001"));
    QVERIFY(GUIUtil::parseMassGridURI(uri, &rv));
    QVERIFY(rv.address == QString("mfb4XJGyaBwNK2Lf4a7r643U3JotRYNw2T"));
    QVERIFY(rv.label == QString());
    QVERIFY(rv.amount == 100000);

    uri.setUrl(QString("massgrid:mfb4XJGyaBwNK2Lf4a7r643U3JotRYNw2T?amount=1.001"));
    QVERIFY(GUIUtil::parseMassGridURI(uri, &rv));
    QVERIFY(rv.address == QString("mfb4XJGyaBwNK2Lf4a7r643U3JotRYNw2T"));
    QVERIFY(rv.label == QString());
    QVERIFY(rv.amount == 100100000);

    uri.setUrl(QString("massgrid:mfb4XJGyaBwNK2Lf4a7r643U3JotRYNw2T?amount=100&label=Some Example"));
    QVERIFY(GUIUtil::parseMassGridURI(uri, &rv));
    QVERIFY(rv.address == QString("mfb4XJGyaBwNK2Lf4a7r643U3JotRYNw2T"));
    QVERIFY(rv.amount == 10000000000LL);
    QVERIFY(rv.label == QString("Some Example"));

    uri.setUrl(QString("massgrid:mfb4XJGyaBwNK2Lf4a7r643U3JotRYNw2T?message=Some Example Address"));
    QVERIFY(GUIUtil::parseMassGridURI(uri, &rv));
    QVERIFY(rv.address == QString("mfb4XJGyaBwNK2Lf4a7r643U3JotRYNw2T"));
    QVERIFY(rv.label == QString());

    QVERIFY(GUIUtil::parseMassGridURI("massgrid://mfb4XJGyaBwNK2Lf4a7r643U3JotRYNw2T?message=Some Example Address", &rv));
    QVERIFY(rv.address == QString("mfb4XJGyaBwNK2Lf4a7r643U3JotRYNw2T"));
    QVERIFY(rv.label == QString());

    uri.setUrl(QString("massgrid:mfb4XJGyaBwNK2Lf4a7r643U3JotRYNw2T?req-message=Some Example Address"));
    QVERIFY(GUIUtil::parseMassGridURI(uri, &rv));

    uri.setUrl(QString("massgrid:mfb4XJGyaBwNK2Lf4a7r643U3JotRYNw2T?amount=1,000&label=Some Example"));
    QVERIFY(!GUIUtil::parseMassGridURI(uri, &rv));

    uri.setUrl(QString("massgrid:mfb4XJGyaBwNK2Lf4a7r643U3JotRYNw2T?amount=1,000.0&label=Some Example"));
    QVERIFY(!GUIUtil::parseMassGridURI(uri, &rv));

    uri.setUrl(QString("massgrid:mfb4XJGyaBwNK2Lf4a7r643U3JotRYNw2T?amount=100&label=Some Example&message=Some Example Message&IS=1"));
    QVERIFY(GUIUtil::parseMassGridURI(uri, &rv));
    QVERIFY(rv.address == QString("mfb4XJGyaBwNK2Lf4a7r643U3JotRYNw2T"));
    QVERIFY(rv.amount == 10000000000LL);
    QVERIFY(rv.label == QString("Some Example"));
    QVERIFY(rv.message == QString("Some Example Message"));
    QVERIFY(rv.fUseInstantSend == 1);

    uri.setUrl(QString("massgrid:mfb4XJGyaBwNK2Lf4a7r643U3JotRYNw2T?amount=100&label=Some Example&message=Some Example Message&IS=Something Invalid"));
    QVERIFY(GUIUtil::parseMassGridURI(uri, &rv));
    QVERIFY(rv.address == QString("mfb4XJGyaBwNK2Lf4a7r643U3JotRYNw2T"));
    QVERIFY(rv.amount == 10000000000LL);
    QVERIFY(rv.label == QString("Some Example"));
    QVERIFY(rv.message == QString("Some Example Message"));
    QVERIFY(rv.fUseInstantSend != 1);

    uri.setUrl(QString("massgrid:mfb4XJGyaBwNK2Lf4a7r643U3JotRYNw2T?IS=1"));
    QVERIFY(GUIUtil::parseMassGridURI(uri, &rv));
    QVERIFY(rv.fUseInstantSend == 1);

    uri.setUrl(QString("massgrid:mfb4XJGyaBwNK2Lf4a7r643U3JotRYNw2T?IS=0"));
    QVERIFY(GUIUtil::parseMassGridURI(uri, &rv));
    QVERIFY(rv.fUseInstantSend != 1);

    uri.setUrl(QString("massgrid:mfb4XJGyaBwNK2Lf4a7r643U3JotRYNw2T"));
    QVERIFY(GUIUtil::parseMassGridURI(uri, &rv));
    QVERIFY(rv.fUseInstantSend != 1);
}
