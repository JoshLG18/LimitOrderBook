// File to simulate a order book

// Import all libraries
#include "orderbook.h"

using namespace std;

int main() {
    OrderBook orderBook;

    while (true) {
        cout << "\nWhat do you want to do?" << endl;
        cout << "1. Add order" << endl;
        cout << "2. Cancel order" << endl;
        cout << "3. Show book" << endl;
        cout << "4. Show trades" << endl;
        cout << "5. Show analytics" << endl;
        cout << "6. Quit" << endl;
        cout << "Enter choice: ";
        
        int choice;
        cin >> choice;
        
        if (choice == 1) {
            // ask for order details
            cout << "Order ID: " << endl;
            int id;
            cin >> id;

            cout << "Side (1=BUY, 2=SELL): " << endl;
            int sideChoice;
            cin >> sideChoice;
            OrderSide side = (sideChoice == 1) ? OrderSide::BUY : OrderSide::SELL;

            cout << "Type (1=LIMIT, 2=MARKET): " << endl;
            int typeChoice;
            cin >> typeChoice;
            OrderType type = (typeChoice == 1) ? OrderType::LIMIT : OrderType::MARKET;

            double price = 0.0;
            if (type == OrderType::LIMIT) {
                cout << "Price: " << endl;
                cin >> price;
            }

            cout << "Order Quantity: " << endl;
            int64_t quantity;
            cin >> quantity;

            Order order(id, side, toTicks(price), quantity, type);
            orderBook.addOrder(order);
            
        } else if (choice == 2) {
            // ask for order id
            cout << "Enter the ID of the order you would like to cancel: " << endl;
            int id;
            cin >> id;

            orderBook.cancelOrder(id);

        } else if (choice == 3) {

            orderBook.showOrderBook();

        } else if (choice == 4) {

            orderBook.showTrades();

        } else if (choice == 5) {

            orderBook.analytics.displaySummary(orderBook.getBestBid(), orderBook.getBestAsk());

        } else if (choice == 6) {
            break;
        }
        // etc
    }
    return 0;
}
