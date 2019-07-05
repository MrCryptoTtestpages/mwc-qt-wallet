#include "k_AccountTransfer.h"
#include "../wallet/wallet.h"
#include "../windows/k_accounttransfer_w.h"
#include "../core/windowmanager.h"
#include "../core/appcontext.h"
#include "../state/statemachine.h"

namespace state {

AccountTransfer::AccountTransfer(const StateContext &context) :
        State(context, STATE::ACCOUNT_TRANSFER) {

    // using static connection. Lock flag transferInProgress will be used to switch the processing

    connect( context.wallet, &wallet::Wallet::onSetReceiveAccount, this, &AccountTransfer::onSetReceiveAccount, Qt::QueuedConnection );
    connect( context.wallet, &wallet::Wallet::onSend, this, &AccountTransfer::onSend, Qt::QueuedConnection );
    connect( context.wallet, &wallet::Wallet::onSlateSend, this, &AccountTransfer::onSlateSend, Qt::QueuedConnection );
    connect( context.wallet, &wallet::Wallet::onSlateFinalized, this, &AccountTransfer::onSlateFinalized, Qt::QueuedConnection );
    connect( context.wallet, &wallet::Wallet::onWalletBalanceUpdated, this, &AccountTransfer::onWalletBalanceUpdated, Qt::QueuedConnection );
}

AccountTransfer::~AccountTransfer() {

}

NextStateRespond AccountTransfer::execute() {
    if (context.appContext->getActiveWndState() != STATE::ACCOUNT_TRANSFER)
        return NextStateRespond(NextStateRespond::RESULT::DONE);

    wnd = new wnd::AccountTransfer( context.wndManager->getInWndParent(), this );
    context.wndManager->switchToWindow(wnd);

    return NextStateRespond( NextStateRespond::RESULT::WAIT_FOR_ACTION );
}


// get balance for current account
QVector<wallet::AccountInfo> AccountTransfer::getWalletBalance() {
    return context.wallet->getWalletBalance();
}

core::SendCoinsParams AccountTransfer::getSendCoinsParams() {
    return context.appContext->getSendCoinsParams();
}

void AccountTransfer::updateSendCoinsParams(const core::SendCoinsParams &params) {
    context.appContext->setSendCoinsParams(params);
}


// nanoCoins < 0 - all funds
void AccountTransfer::transferFunds(const wallet::AccountInfo & accountFrom,
                        const wallet::AccountInfo & accountTo,
                        int64_t nanoCoins) {
    Q_ASSERT(wnd);
    if (!wnd)
        return;

    if (transferState>=0) {
        wnd-> showTransferResults(false, "Another funds transfer operation is in the progress. We can't start a new transfer.");
        return;
    }

    myAddress = context.wallet->getLastKnownMwcBoxAddress();

    // mwc mq expected to be online, we will use it for slate exchange
    if (myAddress.isEmpty() || !context.wallet->getListeningStatus().first) {
        wnd->showTransferResults(false, "Please turn on mwc mq listener. We can't transfer funds in offline mode");
        return;
    }

    // Expected that everything is fine, but will do operation step by step
    // 1. set-recv to accountTo
    // 3. Send funds (will switch to account).
    // 4. Wait for 'finalize' step
    // 6. Restore back receive account
    // 7. Restore back current account
    // 5. Refresh accounts balance

    trAccountFrom = accountFrom;
    trAccountTo = accountTo;
    trNanoCoins = nanoCoins;
    trSlate = "";

    transferState = 0;
    context.wallet->setReceiveAccount( trAccountTo.accountName );
}

void AccountTransfer::goBack() {
    context.stateMachine->setActionWindow( STATE::ACCOUNTS );
}

// set receive account name results
void AccountTransfer::onSetReceiveAccount( bool ok, QString AccountOrMessage ) {
    if (transferState!=0)
        return;

    if  (!ok) {
        if (wnd)
            wnd->showTransferResults(false, "Failed to set receive account. " + AccountOrMessage);
        transferState = -1;
        return;
    }

    transferState=1;

    core::SendCoinsParams prms = context.appContext->getSendCoinsParams();
    context.wallet->sendTo( trAccountFrom, trNanoCoins, myAddress, "", prms.inputConfirmationNumber, prms.changeOutputs );
}


void AccountTransfer::onSend( bool success, QStringList errors ) {
    if (transferState!=1)
        return;

    if  (!success) {
        if (wnd)
            wnd->showTransferResults(false, "Failed to send the funds. " + util::formatErrorMessages(errors) );
        transferState = -1;
        return;
    }

    // Waiting for finalized slate to continue
}

void AccountTransfer::onSlateSend( QString slate, QString mwc, QString sendAddr ) {
    Q_UNUSED(mwc);
    Q_UNUSED(sendAddr);
    if (transferState!=1)
        return;

    trSlate = slate;
}

void AccountTransfer::onSlateFinalized( QString slate ) {
    if (transferState!=1)
        return;

    if (slate == trSlate) {
        // can go forward. Restore the state and update the balance

        transferState=2;

        context.wallet->setReceiveAccount( context.appContext->getReceiveAccount() );
        context.wallet->updateWalletBalance();
    }
}

void AccountTransfer::onWalletBalanceUpdated() {
    if (transferState!=2)
        return;

    if (wnd)
        wnd->showTransferResults(true, "" );

    // Done, finally
    transferState = -1;
}



}
