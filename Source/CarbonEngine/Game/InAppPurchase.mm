/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"

#ifdef APPLE

#include "CarbonEngine/Game/InAppPurchase.h"

#undef new
#include <StoreKit/StoreKit.h>
#include "CarbonEngine/Core/Memory/MemoryInterceptor.h"

@interface ProductsRequestDelegate : NSObject <SKProductsRequestDelegate>
@property (atomic) bool* isInitialized;
@property (atomic) std::vector<SKProduct*>* products;
@end

@implementation ProductsRequestDelegate
@synthesize isInitialized;
@synthesize products;

- (void)productsRequest:(SKProductsRequest*)request didReceiveResponse:(SKProductsResponse*)response
{
    // Report any errors
    for (NSString* identifier in response.invalidProductIdentifiers)
        LOG_ERROR_WITHOUT_CALLER << "Unrecognized in-app purchase product: " << identifier;

    // Store loaded products
    self.products->clear();
    for (SKProduct* product in response.products)
    {
        self.products->push_back(product);
        LOG_INFO << "Initialized in-app purchase product: " << product.productIdentifier;
    }

    *self.isInitialized = true;

    // Restore previously completed transactions
    [[SKPaymentQueue defaultQueue] restoreCompletedTransactions];
}

- (void)request:(SKRequest*)request didFailWithError:(NSError*)error
{
    LOG_ERROR_WITHOUT_CALLER << "In-app purchase product request failed with error: " << [error localizedDescription];
}
@end

@interface PaymentTransactionObserver : NSObject <SKPaymentTransactionObserver>
@property (atomic) Carbon::InAppPurchase* inAppPurchase;
@end

@implementation PaymentTransactionObserver
@synthesize inAppPurchase;

- (void)paymentQueue:(SKPaymentQueue*)queue updatedTransactions:(NSArray*)transactions
{
    for (SKPaymentTransaction* transaction in transactions)
    {
        Carbon::InAppPurchase::TransactionDetails::State state;
        if (transaction.transactionState == SKPaymentTransactionStatePurchasing)
            state = Carbon::InAppPurchase::TransactionDetails::Purchasing;
        else if (transaction.transactionState == SKPaymentTransactionStatePurchased)
            state = Carbon::InAppPurchase::TransactionDetails::Purchased;
        else if (transaction.transactionState == SKPaymentTransactionStateFailed)
            state = Carbon::InAppPurchase::TransactionDetails::Failed;
        else if (transaction.transactionState == SKPaymentTransactionStateRestored)
            state = Carbon::InAppPurchase::TransactionDetails::Restored;
        else
            continue;

        LOG_INFO << "In-app purchase transaction updated, product: " << transaction.payment.productIdentifier
                 << ", state: " << state;

        // Fire event to notify the application of the transaction state change
        self.inAppPurchase->onTransactionUpdated.fireWith(transaction.payment.productIdentifier, state);

        // Complete the transaction unless it is currently purchasing
        if (state != Carbon::InAppPurchase::TransactionDetails::Purchasing)
            [[SKPaymentQueue defaultQueue] finishTransaction:transaction];
    }
}
@end

namespace Carbon
{

class InAppPurchase::Members
{
public:

    bool isInitialized = false;

    std::vector<SKProduct*> products;

    ProductsRequestDelegate* productsRequestDelegate = nil;

    PaymentTransactionObserver* paymentTransactionObserver = nil;
};

InAppPurchase::InAppPurchase() : onTransactionUpdated(this)
{
    m = new Members;

    m->productsRequestDelegate = [[ProductsRequestDelegate alloc] init];
    m->productsRequestDelegate.isInitialized = &m->isInitialized;
    m->productsRequestDelegate.products = &m->products;

    m->paymentTransactionObserver = [[PaymentTransactionObserver alloc] init];
    m->paymentTransactionObserver.inAppPurchase = this;
    [[SKPaymentQueue defaultQueue] addTransactionObserver:m->paymentTransactionObserver];
}

InAppPurchase::~InAppPurchase()
{
    delete m;
    m = nullptr;
}

void InAppPurchase::initialize(const Vector<String>& productIdentifiers)
{
    m->products.clear();
    m->isInitialized = false;

    auto ids = [[NSMutableArray alloc] init];
    for (auto& identifier : productIdentifiers)
        [ids addObject:identifier.toNSString()];

    auto request = [[SKProductsRequest alloc] initWithProductIdentifiers:[NSSet setWithArray:ids]];

    request.delegate = m->productsRequestDelegate;

    LOG_INFO << "Requesting in-app purchase data, products: " << productIdentifiers;
    [request start];
}

bool InAppPurchase::isInitialized() const
{
    return m->isInitialized;
}

Vector<String> InAppPurchase::getProducts() const
{
    auto products = Vector<String>();

    for (auto& product : m->products)
        products.append(product.productIdentifier);

    return products;
}

UnicodeString InAppPurchase::getProductTitle(const String& productIdentifier) const
{
    for (auto& product : m->products)
    {
        if (String(product.productIdentifier) == productIdentifier)
            return product.localizedTitle;
    }

    return {};
}

UnicodeString InAppPurchase::getProductDescription(const String& productIdentifier) const
{
    for (auto& product : m->products)
    {
        if (String(product.productIdentifier) == productIdentifier)
            return product.localizedDescription;
    }

    return {};
}

UnicodeString InAppPurchase::getProductPrice(const String& productIdentifier) const
{
    for (auto& product : m->products)
    {
        if (String(product.productIdentifier) == productIdentifier)
        {
            auto numberFormatter = [[NSNumberFormatter alloc] init];

            [numberFormatter setFormatterBehavior:NSNumberFormatterBehavior10_4];
            [numberFormatter setNumberStyle:NSNumberFormatterCurrencyStyle];
            [numberFormatter setLocale:product.priceLocale];

            return [numberFormatter stringFromNumber:product.price];
        }
    }

    return {};
}

bool InAppPurchase::purchase(const String& productIdentifier)
{
    // Check we can do a purchase
    if (!isInitialized() || ![SKPaymentQueue canMakePayments])
        return false;

    for (const auto& product : m->products)
    {
        if (String(product.productIdentifier) == productIdentifier)
        {
            LOG_INFO << "Initiating in-app purchase of product: " << productIdentifier;

            // Attempt to make a purchase of this product, the result will be returned by InAppPurchase::onTransactionCompleted
            [[SKPaymentQueue defaultQueue] addPayment:[SKPayment paymentWithProduct:product]];

            return true;
        }
    }

    return false;
}

}

#endif
