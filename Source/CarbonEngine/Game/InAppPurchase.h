/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#ifdef APPLE

#include "CarbonEngine/Core/EventDelegate.h"

namespace Carbon
{

/**
 * Wrapper class for integrating in-app purchase functionality on iOS and macOS.
 */
class CARBON_API InAppPurchase : private Noncopyable
{
public:

    InAppPurchase();
    ~InAppPurchase();

    /**
     * Holds details on an in-app purchase transaction, this includes the identifier of an in-app purchase product and
     * the current state of the transaction. After a call to InAppPurchase::purchase() the
     * InAppPurchase::onTransactionUpdated event will fire with details on the in-progress transaction, applications
     * must handle this event in order to respond to in-app purchases made by the user.
     */
    class TransactionDetails
    {
    public:

        /**
         * The possible states that an in-app purchase transaction can be in.
         */
        enum State
        {
            /**
             * The in-app purchase is currently underway.
             */
            Purchasing,

            /**
             * The in-app purchase transaction completed successfully.
             */
            Purchased,

            /**
             * The in-app purchase transaction failed.
             */
            Failed,

            /**
             * The in-app purchase transaction was restored, i.e. the product has been purchased previously by the
             * current user. Previously purchased products are restored automatically as part of in-app purchase
             * initialization.
             */
            Restored
        };

        /**
         * Returns the identifier of the product that this transaction applies to.
         */
        const String& getProductIdentifier() const { return productIdentifier_; }

        /**
         * Returns the new state of the transaction. When th
         */
        State getState() const { return state_; }

        /**
         * Constructs with the specified product identifier and state.
         */
        TransactionDetails(String productIdentifier, State state)
            : productIdentifier_(std::move(productIdentifier)), state_(state)
        {
        }

    private:

        const String productIdentifier_;
        const State state_;
    };

    /**
     * This event is fired when an in-app purchase transaction changes state, the details of the new transaction state
     * are specified by the passed InAppPurchase::TransactionDetails instance. When the state is \a Purchased or
     * \a Restored the application should make the corresponding content available to the user.
     */
    EventDispatcher<InAppPurchase, const TransactionDetails&> onTransactionUpdated;

    /**
     * Initializes this in-app purchase instance for use with the specified product identifiers, initialization itself
     * is asynchronous and the result can be checked using InAppPurchase::isInitialized().
     */
    void initialize(const Vector<String>& productIdentifiers);

    /**
     * Returns whether or not in-app purchase has initialized successfully following a call to
     * InAppPurchase::initialize().
     */
    bool isInitialized() const;

    /**
     * Returns all the currently initialized products.
     */
    Vector<String> getProducts() const;

    /**
     * Returns the localized title for the product with the specified identifier.
     */
    UnicodeString getProductTitle(const String& productIdentifier) const;

    /**
     * Returns the localized description for the product with the specified identifier.
     */
    UnicodeString getProductDescription(const String& productIdentifier) const;

    /**
     * Returns the localized price string for the product with the specified identifier.
     */
    UnicodeString getProductPrice(const String& productIdentifier) const;

    /**
     * Initiates a purchase of the specified product, the result is returned through the
     * InAppPurchase::onTransactionUpdated event. If the product identifier is unknown then this method returns false.
     */
    bool purchase(const String& productIdentifier);

private:

    class Members;
    Members* m = nullptr;
};

}

#endif
