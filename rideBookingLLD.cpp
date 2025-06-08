#include <iostream>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <string>
#include <cctype>
#include <thread>
#include <chrono>

using namespace std;

class GeoLocationManager;
class PaymentGateway;

// ------------------------ User & Driver Classes ------------------------

class Driver {
public:
    string userType = "driver";
    string name;
    string vehicleType;
    string currentLocation;
    bool availability;

    Driver(string name, string vehicleType) {
        this->name = name;
        this->vehicleType = vehicleType;
        this->currentLocation = "";
        this->availability = false;
    }
};

class User {
public:
    string userType = "user";
    string name;
    string phno;
    string currentLocation;
    bool isOnline;

    User(string name, string phno) {
        this->name = name;
        this->phno = phno;
        this->currentLocation = "";
        this->isOnline = false;
    }
};

// ------------------------ auth,user,driver managers ------------------------

class userManager {
public:
    vector<User*> users;

    void addUser(User* user) {
        users.push_back(user);
    }

    User* getUser(string username) {
        for (auto* user : users) {
            if (user->name == username) return user;
        }
        return nullptr;
    }
    ~userManager() {
        for (User* u : users) {
            delete u;
        }
        users.clear();
    }
};

class driverManager {
public:
    vector<Driver*> drivers;

    void addDriver(Driver* driver) {
        drivers.push_back(driver);
    }

    Driver* getDriver(string username) {
        for (auto* driver : drivers) {
            if (driver->name == username) return driver;
        }
        return nullptr;
    }
    ~driverManager() {
        for (Driver* d : drivers) {
            delete d;
        }
        drivers.clear();
    }
};

class AuthManager {
private:
    userManager* um;
    driverManager* dm;

public:
    AuthManager(userManager* um, driverManager* dm) {
        this->um = um;
        this->dm = dm;
    }

    void login(string username) {
        if (User* u = um->getUser(username)) {
            u->isOnline = true;
            cout << "User " << u->name << " logged in.\n";
        } else if (Driver* d = dm->getDriver(username)) {
            d->availability = true;
            cout << "Driver " << d->name << " logged in.\n";
        } else {
            cout << "Login failed for " << username << "\n";
        }
    }
};

// ------------------------ GeoLocationManager to manage driver and user location ------------------------

class GeoLocationManager {
public:
    unordered_map<string, string> usersLocations;
    unordered_map<string, string> driverLocations;

    void storeLocation(string name, string userType, string location) {
        if (userType == "driver") {
            driverLocations[name] = location;
        } else if (userType == "user") {
            usersLocations[name] = location;
        }
    }

    string getDriverLocation(string name) {
        if (driverLocations.find(name) != driverLocations.end()) {
            return driverLocations[name];
        } else {
            return "Driver not found";
        }
    }

    void updateDriverLocation(string driverName, string newLocation) {
        driverLocations[driverName] = newLocation;
        cout << "[GeoManager] Driver " << driverName << " moved to " << newLocation << endl;
    }
};

// ------------------------location Observer Interfaces(polling and socketConnection) ------------------------

class iLocationObserver {
public:
    GeoLocationManager* geoManager;
    iLocationObserver(GeoLocationManager* m) {
        this->geoManager = m;
    }
    virtual void updateLocation(string name, string location, string userType) = 0;
    virtual ~iLocationObserver() {}
};

class polling : public iLocationObserver {
public:
    polling(GeoLocationManager* m) : iLocationObserver(m) {}

    void updateLocation(string name, string location, string userType) override {
        cout << "[Polling] " << userType << " " << name << " is at " << location << "\n";
        geoManager->storeLocation(name, userType, location);
    }
};

class socketConnection : public iLocationObserver {
public:
    socketConnection(GeoLocationManager* m) : iLocationObserver(m) {}

    void updateLocation(string name, string location, string userType) override {
        cout << "[Socket] " << userType << " " << name << " moved to " << location << "\n";
        geoManager->storeLocation(name, userType, location);
    }
};

// ------------------------ location observable the handle location observer classes ------------------------

class iUserStatus {
public:
    vector<iLocationObserver*> observers;
    virtual void addObserver(iLocationObserver* o) = 0;
    virtual void removeObserver(iLocationObserver* o) = 0;
    virtual void notify(string name, string location, string userType) = 0;
    virtual ~iUserStatus() {
        for(auto obs : observers) {
            delete obs;
        }
        observers.clear();
    }
};

//concrete location observable
class statusListner : public iUserStatus {
public:
    void addObserver(iLocationObserver* o) override {
        observers.push_back(o);
    }

    void removeObserver(iLocationObserver* o) override {
        observers.erase(remove(observers.begin(), observers.end(), o), observers.end());
    }

    void notify(string name, string location, string userType) override {
        for (auto& observer : observers) {
            observer->updateLocation(name, location, userType);
        }
    }
};

//-----------------------ride booking flow-----------------------------
// --------------------- Ride Object -------------------------
class RideObject {
public:
    string start;
    string dest;
    string name; // User's name
    string rideStatus;
    string rideType;
    string vehicle;
    string vehicleType;
    string driverName;
    int fare; 

    RideObject(string start = "", string dest = "", string name = "", string vehicle = "", string vehicleType = "") {
        this->start = start;
        this->dest = dest;
        this->name = name;
        this->rideStatus = "pending";
        this->rideType = "normal";
        this->vehicle = vehicle;
        this->vehicleType = vehicleType;
        this->driverName = "";
        this->fare = 0;
    }
};

// --------------------- IBooking Interface -------------------------
class iBooking {
public:
    RideObject* r;
    iBooking(RideObject* r) {
        this->r = r;
    }

    virtual void book(RideObject* r) = 0;
    virtual ~iBooking() {}
};

// --------------------- Concrete Booking Class -------------------------
class bookRide : public iBooking {
public:
    bookRide(RideObject* r) : iBooking(r) {}

    void book(RideObject* r) override {
        cout << "Enter your name: ";
        cin >> r->name;

        cout << "Enter pickup location: ";
        cin >> r->start;

        cout << "Enter destination: ";
        cin >> r->dest;

        cout << "Enter ride type (normal/pooling): ";
        cin >> r->rideType;

        cout << "Enter vehicle (sedan/suv/auto): ";
        cin >> r->vehicle;
    }
};

// --------------------- Ride Type Factory Interface -------------------------
class vehicleTypeFactory {
public:
    virtual void createBooking(RideObject* r) = 0;
    virtual ~vehicleTypeFactory() {}
};

// --------------------- Concrete Ride Type Factories -------------------------
class normalRideFactory : public vehicleTypeFactory {
public:
    void createBooking(RideObject* r) override {
        r->rideType = "normal";
        cout << "Normal booking created for you.\n";
    }
};

class poolingRideFactory : public vehicleTypeFactory {
public:
    void createBooking(RideObject* r) override {
        r->rideType = "pooling";
        cout << "Pooling booking created for you.\n";
    }
};

// New Interface for selecting RideTypeFactory
class IRideTypeFactorySelector {
public:
    virtual vehicleTypeFactory* selectRideTypeFactory(const string& rideType) = 0;
    virtual ~IRideTypeFactorySelector() {}
};

// Concrete implementation for selecting RideTypeFactory
class RideTypeFactorySelector : public IRideTypeFactorySelector {
public:
    vehicleTypeFactory* selectRideTypeFactory(const string& rideType) override {
        if (rideType == "normal") {
            return new normalRideFactory();
        } else {
            return new poolingRideFactory();
        }
    }
};


// --------------------- Vehicle Factory Interface -------------------------
class iVehicleTypeFactory {
public:
    virtual void bookVehicle(RideObject* r) = 0;
    virtual ~iVehicleTypeFactory() {}
};

// --------------------- Concrete Vehicle Factories -------------------------
class Car : public iVehicleTypeFactory {
public:
    void bookVehicle(RideObject* r) override {
        r->vehicle = "car";
        cout << "Car vehicle booked.\n";
    }
};

class Sedan : public Car {
public:
    void bookVehicle(RideObject* r) override {
        r->vehicle = "car";
        r->vehicleType = "sedan";
        cout << "Sedan booked.\n";
    }
};

class SUV : public Car {
public:
    void bookVehicle(RideObject* r) override {
        r->vehicle = "car";
        r->vehicleType = "suv";
        cout << "SUV booked.\n";
    }
};

class Auto : public iVehicleTypeFactory {
public:
    void bookVehicle(RideObject* r) override {
        r->vehicle = "auto";
        r->vehicleType = "3-wheeler";
        cout << "Auto vehicle booked.\n";
    }
};

//Interface for selecting VehicleTypeFactory
class IVehicleFactorySelector {
public:
    virtual iVehicleTypeFactory* selectVehicleFactory(const string& vehicle) = 0;
    virtual ~IVehicleFactorySelector() {}
};

// Concrete implementation for selecting VehicleTypeFactory
class VehicleFactorySelector : public IVehicleFactorySelector {
public:
    iVehicleTypeFactory* selectVehicleFactory(const string& vehicle) override {
        if (vehicle == "sedan") {
            return new Sedan();
        } else if (vehicle == "suv") {
            return new SUV();
        } else {
            return new Auto();
        }
    }
};

// ----------------------price calculation for a fare-----------------------
class iPriceInterface {
public:
    virtual int calculate(RideObject* r) = 0;
    virtual ~iPriceInterface() {}
};

class normalPrice : public iPriceInterface {
public:
    int calculate(RideObject* r) override {
        int base = 50;
        cout << "Normal pricing applied.\n";
        return base;
    }
};

class peakHours : public iPriceInterface {
public:
    int calculate(RideObject* r) override {
        int base = 50;
        int peakSurcharge = 20;
        cout << "Peak hour pricing applied.\n";
        return base + peakSurcharge;
    }
};

// Now this is a pure strategy executor, not responsible for creating the strategy
class PriceStrategy {
public:
    iPriceInterface* p;

    // PriceStrategy no longer owns 'p', it's injected
    PriceStrategy(iPriceInterface* p_strategy) {
        this->p = p_strategy;
    }

    int calFare(RideObject* r) {
        return p->calculate(r);
    }
};

// New Interface for Price Calculation, to be injected into BookingManager
class IPriceCalculator {
public:
    virtual int calculateFare(RideObject* r) = 0;
    virtual ~IPriceCalculator() {}
};

class ConcretePriceCalculator : public IPriceCalculator {
public:
    int calculateFare(RideObject* r) override {
        iPriceInterface* pricing = new normalPrice(); // This still creates concrete, could be factory
        PriceStrategy strategy(pricing);
        int fare = strategy.calFare(r);
        delete pricing;
        return fare;
    }
};


//notify the ride allocation factory that a new ride object is created and they use suitable stratergy to allocate driver
class iBookingObserver {
public:
    virtual void notifyBookingDetails(RideObject* r) = 0;
    virtual ~iBookingObserver() {}
};

class iBookingSubject {
protected:
    vector<iBookingObserver*> observers;
public:
    virtual void addObservers(iBookingObserver* obs) = 0;
    virtual void removeObservers(iBookingObserver* obs) = 0;
    virtual void notify(RideObject* r) = 0;
    virtual ~iBookingSubject() {
        for(auto obs : observers) {
            delete obs;
        }
        observers.clear();
    }
};

class BookingSubject : public iBookingSubject {
public:
    void addObservers(iBookingObserver* obs) override {
        observers.push_back(obs);
    }

    void removeObservers(iBookingObserver* obs) override {
        observers.erase(remove(observers.begin(), observers.end(), obs), observers.end());
    }

    void notify(RideObject* r) override {
        for (auto obs : observers) {
            obs->notifyBookingDetails(r);
        }
    }
};

class iDriverAllocationStratergy {
public:
    virtual void match(RideObject* r, string drivername) = 0;
    virtual void getDriver(string uname) = 0;
    virtual ~iDriverAllocationStratergy() {}
};

class nearestDriver : public iDriverAllocationStratergy {
public:
    void match(RideObject* r, string drivername) override {
        cout << "Matching nearest driver...\n";
        // In a real system, you'd find a driver here based on location
        // For now, let's assign a dummy driver or the one passed if any
        if (drivername.empty()) {
            r->driverName = "Alice"; // Dummy driver
        } else {
            r->driverName = drivername;
        }
        r->rideStatus = "confirmed";
    }

    void getDriver(string uname) override {
        cout << "Driver allocated: " << uname << "\n";
    }
};

class highestRating : public iDriverAllocationStratergy {
public:
    void match(RideObject* r, string drivername) override {
        cout << "Matching highest rated driver...\n";
        // In a real system, you'd find a driver here based on rating
        if (drivername.empty()) {
            r->driverName = "Bob"; // Dummy driver
        } else {
            r->driverName = drivername;
        }
        r->rideStatus = "confirmed";
    }

    void getDriver(string uname) override {
        cout << "Driver allocated: " << uname << "\n";
    }
};

class rideAllocationFactory {
public:
    iDriverAllocationStratergy* st;
    rideAllocationFactory(iDriverAllocationStratergy* st) {
        this->st = st;
    }

    void allocateDriver(RideObject* r) {
        st->match(r, ""); // Driver name is chosen by strategy, empty string implies strategy will pick
    }
    ~rideAllocationFactory() {
        delete st;
    }
};


//notification system/service....................................................................
//------------------ Strategy ---------------------
class iNotificationStrategy {
public:
    virtual void sendMessage(string message, string recipient, string recipientType) = 0;
    virtual ~iNotificationStrategy() {}
};

class Email : public iNotificationStrategy {
public:
    void sendMessage(string message, string recipient, string recipientType) override {
        cout << "[EMAIL to " << recipientType << " - " << recipient << "]: " << message << endl;
    }
};

class PushNotification : public iNotificationStrategy {
public:
    void sendMessage(string message, string recipient, string recipientType) override {
        cout << "[PUSH to " <<  recipientType << " - " << recipient << "]: " << message << endl;
    }
};

//------------------ iNotification Interface ---------------------
class iNotification {
public:
    virtual void send(string message, string messageType, RideObject* r) = 0;
    virtual ~iNotification() {}
};

//------------------ Base Notification ---------------------
class BaseNotification : public iNotification {
protected:
    iNotificationStrategy* strategy;
public:
    BaseNotification(iNotificationStrategy* strategy) {
        this->strategy = strategy;
    }

    void send(string message, string messageType, RideObject* r) override {
        // This base implementation assumes 'r->name' is the primary recipient for the RideObject
        // Specific decorators/methods will handle notifying drivers if needed.
        strategy->sendMessage(message, r->name, "user"); // Defaulting to user for this method
    }
    virtual ~BaseNotification() {
        delete strategy;
    }
};

//------------------ Decorator ---------------------
class NotificationDecorator : public iNotification {
protected:
    iNotification* wrapped;
public:
    NotificationDecorator(iNotification* wrapped) {
        this->wrapped = wrapped;
    }

    virtual void send(string message, string messageType, RideObject* r) {
        wrapped->send(message, messageType, r);
    }
    virtual ~NotificationDecorator() {
        delete wrapped;
    }
};

class RideAcceptedNotif : public NotificationDecorator {
public:
    RideAcceptedNotif(iNotification* wrapped) : NotificationDecorator(wrapped) {}

    void send(string message, string messageType, RideObject* r) override {
        cout << ">> Ride Accepted Notification Triggered (Auto-Accepted).\n";
        // Automatically accept the ride - this logic is now handled by RideRequestManager potentially,
        // or this decorator confirms a pre-accepted state.
        bool res = true; // For demonstration, still auto-accepting
        if (res) {
            wrapped->send("Your ride is accepted by driver " + r->driverName + ". Driver is on the way to " + r->start, "rideAccepted", r);
            r->rideStatus = "driver_on_the_way";
        } else {
            cout << "Driver rejected the ride.\n";
            r->rideStatus = "driver_rejected";
        }
    }
};

//------------------ Observer Pattern ---------------------
class iNotificationObserver {
public:
    virtual void update(string message, string recipient) = 0;
    virtual ~iNotificationObserver() {}
};

class UserNotificationObserver : public iNotificationObserver {
public:
    void update(string message, string recipient) override {
        cout << "[User Observer] Notified " << recipient << ": " << message << endl;
    }
};

class DriverNotificationObserver : public iNotificationObserver {
public:
    void update(string message, string recipient) override {
        cout << "[Driver Observer] Notified " << recipient << ": " << message << endl;
    }
};

class NotificationSubject {
    vector<iNotificationObserver*> observers;
public:
    void addObserver(iNotificationObserver* obs) {
        observers.push_back(obs);
    }

    void notifyAll(string message, string recipient) {
        for (auto& obs : observers) {
            obs->update(message, recipient);
        }
    }
    ~NotificationSubject() {
        for (auto obs : observers) {
            delete obs;
        }
        observers.clear();
    }
};

//------------------ Factory ---------------------
class NotificationFactory {
public:
    static iNotification* createNotification(string eventType, iNotificationStrategy* strategy) {
        iNotification* base = new BaseNotification(strategy);
        if (eventType == "rideAccepted") {
            return new RideAcceptedNotif(base);
        } else if (eventType == "driverArrived" || eventType == "rideCompleted" ||
                   eventType == "custom_message" || eventType == "driver_notification" ||
                   eventType == "user_notification") {
            // These will use the base notification, possibly with a custom message
            return new BaseNotification(strategy);
        }
        return base;
    }
};

//------------------ Notification Engine ---------------------
class NotificationEngine {
private:
    NotificationSubject* subject;
public:
    NotificationEngine(NotificationSubject* subject) {
        this->subject = subject;
    }

    void notify(string eventType, RideObject* r, string customMessage = "") {
        iNotificationStrategy* strategy = new Email(); // Default to Email, can be dynamically chosen

        iNotification* notif = NotificationFactory::createNotification(eventType, strategy);

        string messageToUse = customMessage.empty() ? "Ride event occurred" : customMessage;

        if (eventType == "rideAccepted") {
            notif->send(messageToUse, eventType, r); // RideAcceptedNotif handles specific logic
        } else if (eventType == "driverArrived") {
            notif->send(messageToUse, eventType, r); // Sends to user by default via BaseNotification (r->name)
            subject->notifyAll("Observer: Driver " + r->driverName + " has arrived at " + r->start, r->name);
        } else if (eventType == "rideCompleted") {
            notif->send(messageToUse, eventType, r); // Sends to user by default via BaseNotification (r->name)
            subject->notifyAll("Observer: Your ride with " + r->driverName + " to " + r->dest + " is completed.", r->name);
            // Driver notification is handled by a separate notifyDriver call from RideManager.
        } else if (eventType == "custom_message") { // Generic custom message to user
             notif->send(messageToUse, eventType, r); // Sends to user by default via BaseNotification (r->name)
             subject->notifyAll("Observer: " + messageToUse, r->name);
        } else {
            notif->send(messageToUse, eventType, r);
            subject->notifyAll("Observer: " + messageToUse, r->name);
        }

        delete notif;
    }

    //directly notifying the driver
    void notifyDriver(string message, string driverName) {
        iNotificationStrategy* strategy = new PushNotification(); // Drivers might prefer push notifications
        iNotification* notif = new BaseNotification(strategy); // Simple base notification

        // Create a dummy RideObject just to satisfy BaseNotification::send signature.
        // The actual recipient targeting is done via NotificationSubject's notifyAll.
        RideObject dummy_ride_for_driver_notif("", "", driverName);
        notif->send(message, "driver_notification", &dummy_ride_for_driver_notif); // BaseNotification will send to dummy_ride_for_driver_notif.name

        subject->notifyAll("Observer: " + message, driverName); // This is the actual notification to driver observer
        delete notif;
    }

    // For directly notifying a user with a non-ride specific message
    void notifyUser(string message, string userName) {
        iNotificationStrategy* strategy = new PushNotification(); // Users might prefer push notifications
        iNotification* notif = new BaseNotification(strategy);

        RideObject dummy_ride_for_user_notif("", "", userName);
        notif->send(message, "user_notification", &dummy_ride_for_user_notif); // BaseNotification will send to dummy_ride_for_user_notif.name

        subject->notifyAll("Observer: " + message, userName); // This is the actual notification to user observer
        delete notif;
    }
};

// This class will now be responsible for allocating the driver and notifying the ride request manager
// that a driver has been allocated. It itself observes the booking subject.
class DriverAllocationManager : public iBookingObserver {
    rideAllocationFactory* f;
    NotificationEngine* notificationEngine; 
public:
    DriverAllocationManager(rideAllocationFactory* f, NotificationEngine* ne) {
        this->f = f;
        this->notificationEngine = ne;
    }

    void notifyBookingDetails(RideObject* r) override {
        cout << "[DriverAllocationManager] Notified of new booking for " << r->name << ". Attempting to allocate driver...\n";
        f->allocateDriver(r);
        if (r->rideStatus == "confirmed") {
            cout << "[DriverAllocationManager] Driver " << r->driverName << " allocated for ride.\n";
            // Notify the driver that a new booking is available for them
            notificationEngine->notifyDriver("New ride request from " + r->name + " to " + r->dest + ". Please accept.", r->driverName);
        } else {
            cout << "[DriverAllocationManager] Failed to allocate driver for ride.\n";
        }
    }
    ~DriverAllocationManager() {
        delete f;
    }
};


class IDriverAllocationOrchestrator {
public:
    virtual void orchestrate(RideObject* r) = 0;
    virtual ~IDriverAllocationOrchestrator() {}
};

// Concrete Implementation for Driver Allocation Orchestration
// This will now be part of the RideRequestManager's logic
class ConcreteDriverAllocationOrchestrator : public IDriverAllocationOrchestrator {
    NotificationEngine* notificationEngine;
public:
    ConcreteDriverAllocationOrchestrator(NotificationEngine* ne) : notificationEngine(ne) {}

    void orchestrate(RideObject* r) override {
        // This is simplified as the actual allocation happens through the observer now
        // For a true orchestration, it would directly interact with driver management
        // and a driver allocation service.
        // it will call DriverAllocationManager directly to simulate
        // the immediate allocation once booking details are received.
        iDriverAllocationStratergy* strategy = new nearestDriver(); // or highestRating()
        rideAllocationFactory* factory = new rideAllocationFactory(strategy);
        DriverAllocationManager* allocator = new DriverAllocationManager(factory, notificationEngine); // Pass notification engine

        allocator->notifyBookingDetails(r); // Simulate direct call to allocation
        delete allocator;
    }
};


// ------------------------ Payment Gateway Class ------------------------
class PaymentGateway {
public:
    void processPayment(RideObject* ride, int fare) {
        cout << "\n--- Redirecting to Payment Gateway ---\n";
        cout << "User: " << ride->name << endl;
        cout << "Ride from: " << ride->start << " to: " << ride->dest << endl;
        cout << "Amount Due: " << fare << " INR" << endl;
        cout << "Payment processing initiated for Ride ID: " << ride->name + "_" + ride->start + "_" + ride->dest << endl; // Simple unique ID
        // Simulate external payment processing
        std::this_thread::sleep_for(std::chrono::seconds(3));
        cout << "Payment successful! Thank you for riding with us.\n";
        ride->rideStatus = "paid"; // Update ride status to indicate payment
    }
};

// --------------------- Ride Manager -------------------------
class RideManager {
private:
    RideObject* currentRide;
    GeoLocationManager* geoManager;
    NotificationEngine* notificationEngine;
    PaymentGateway* paymentGateway;

public:
    // RideManager now accepts the RideObject and assumes it's ready for live management
    RideManager(RideObject* ride, GeoLocationManager* gm, NotificationEngine* ne, PaymentGateway* pg)
        : currentRide(ride), geoManager(gm), notificationEngine(ne), paymentGateway(pg) {}

    void startRide() {
        if (!currentRide || currentRide->rideStatus != "driver_on_the_way") {
            cout << "[RideManager] Cannot start ride: invalid ride object or status.\n";
            return;
        }

        cout << "\n[RideManager] Starting live ride management for ride to " << currentRide->dest << " with driver " << currentRide->driverName << endl;
        trackDriver();
    }

    void notifyDriver(string message) {
        cout << "[RideManager] Sending notification to driver " << currentRide->driverName << ": " << message << endl;
        notificationEngine->notifyDriver(message, currentRide->driverName);
    }

    void notifyUser(string message) {
        cout << "[RideManager] Sending notification to user " << currentRide->name << ": " << message << endl;
        notificationEngine->notifyUser(message, currentRide->name);
    }

    void cancelRide() {
        cout << "[RideManager] Ride cancelled.\n";
        currentRide->rideStatus = "cancelled";
        notifyUser("Your ride has been cancelled.");
        notifyDriver("The ride for " + currentRide->name + " has been cancelled.");
    }

    void trackDriver() {
        string driverName = currentRide->driverName;
        string userPickup = currentRide->start;
        string userDestination = currentRide->dest;

        // Simulate driver moving towards pickup
        cout << "[Live Ride] Driver " << driverName << " is en route to " << userPickup << ".\n";
        // Simulate intermediate locations (simplified)
        geoManager->updateDriverLocation(driverName, "near_" + userPickup + "_1");
        std::this_thread::sleep_for(std::chrono::seconds(2));
        geoManager->updateDriverLocation(driverName, "near_" + userPickup + "_2");
        std::this_thread::sleep_for(std::chrono::seconds(2)); // Corrected typo here

        // Driver arrives at pickup
        geoManager->updateDriverLocation(driverName, userPickup);
        currentRide->rideStatus = "driver_at_pickup";
        cout << "[Live Ride] Driver " << driverName << " has arrived at " << userPickup << ".\n";
        notificationEngine->notify("driverArrived", currentRide, "Your driver " + currentRide->driverName + " has arrived at " + currentRide->start + ". Please board the vehicle.");
        std::this_thread::sleep_for(std::chrono::seconds(3)); // Wait for user to board

        // Simulate ride in progress
        currentRide->rideStatus = "in_progress";
        cout << "[Live Ride] Ride to " << userDestination << " is in progress.\n";
        // Simulate intermediate locations
        geoManager->updateDriverLocation(driverName, "midway_" + userDestination + "_1");
        std::this_thread::sleep_for(std::chrono::seconds(2));
        geoManager->updateDriverLocation(driverName, "midway_" + userDestination + "_2");
        std::this_thread::sleep_for(std::chrono::seconds(2));

        // Ride completion
        geoManager->updateDriverLocation(driverName, userDestination);
        currentRide->rideStatus = "completed";
        cout << "[Live Ride] Ride to " << userDestination << " completed!\n";
        notificationEngine->notify("rideCompleted", currentRide, "Your ride with " + currentRide->driverName + " has successfully completed.");
        // Explicitly notify driver of ride completion
        notifyDriver("Ride for " + currentRide->name + " to " + currentRide->dest + " completed.");

        //Initiate payment after ride completion
        if (paymentGateway) {
            paymentGateway->processPayment(currentRide, currentRide->fare); // Use fare from RideObject
        } else {
            cout << "[RideManager] Warning: Payment Gateway not configured.\n";
        }
    }
};

// --------------------- RideRequestManager (New Class - Observer for BookingManager) -------------------------
// This class will listen for new ride bookings and orchestrate driver allocation and initial notifications.
class RideRequestManager : public iBookingObserver {
private:
    NotificationEngine* notificationEngine;
    GeoLocationManager* geoManager;
    PaymentGateway* paymentGateway;
    IDriverAllocationOrchestrator* driverAllocationOrchestrator;

public:
    RideRequestManager(NotificationEngine* ne, GeoLocationManager* gm, PaymentGateway* pg, IDriverAllocationOrchestrator* dao)
        : notificationEngine(ne), geoManager(gm), paymentGateway(pg), driverAllocationOrchestrator(dao) {}

    void notifyBookingDetails(RideObject* r) override {
        cout << "\n[RideRequestManager] Received new ride request for " << r->name << ". Initiating driver allocation.\n";

        // Orchestrate driver allocation
        driverAllocationOrchestrator->orchestrate(r);

        if (r->rideStatus == "confirmed") {
            cout << "[RideRequestManager] Driver " << r->driverName << " successfully allocated. Notifying user and starting ride management.\n";
            // Trigger notification after driver allocation (RideAcceptedNotif auto-accepts)
            notificationEngine->notify("rideAccepted", r); // This will change status to "driver_on_the_way"

            if (r->rideStatus == "driver_on_the_way") {
                // Hand over to RideManager for live tracking
                RideManager* rideManager = new RideManager(r, geoManager, notificationEngine, paymentGateway);
                rideManager->startRide();
                delete rideManager;
            }
        } else {
            cout << "[RideRequestManager] Driver allocation failed or ride rejected for " << r->name << ".\n";
            notificationEngine->notifyUser("Unfortunately, we could not find a driver for your ride at this time. Please try again.", r->name);
        }
    }
};


// --------------------- Booking Manager -------------------------
class BookingManager {
private:
    IRideTypeFactorySelector* rideTypeFactorySelector;
    IVehicleFactorySelector* vehicleFactorySelector;
    IPriceCalculator* priceCalculator;
    iBookingSubject* bookingSubject; // Now owns the subject to notify observers

public:
    BookingManager(IRideTypeFactorySelector* rtfs, IVehicleFactorySelector* vfs,
                   IPriceCalculator* pc, iBookingSubject* bs)
        : rideTypeFactorySelector(rtfs), vehicleFactorySelector(vfs),
          priceCalculator(pc), bookingSubject(bs) {}

    void createBooking() {
        RideObject* ride = new RideObject(); // BookingManager creates the RideObject

        bookRide* br = new bookRide(ride); // Handles user input for booking details
        br->book(ride);
        delete br; // Clean up immediately after use

        // Step 2: Choose ride type using injected selector
        vehicleTypeFactory* rideTypeFactory = rideTypeFactorySelector->selectRideTypeFactory(ride->rideType);
        rideTypeFactory->createBooking(ride);
        delete rideTypeFactory;

        // Step 3: Choose vehicle type using injected selector
        iVehicleTypeFactory* vehicleFactory = vehicleFactorySelector->selectVehicleFactory(ride->vehicle);
        vehicleFactory->bookVehicle(ride);
        delete vehicleFactory;

        // Calculate price using injected calculator
        int fare = priceCalculator->calculateFare(ride);
        ride->fare = fare; // Store fare in RideObject

        // Final Summary
        cout << "\nFinal Booking Summary:\n";
        cout << "Name: " << ride->name
             << "\nStart: " << ride->start
             << "\nDestination: " << ride->dest
             << "\nVehicle: " << ride->vehicle
             << "\nVehicle Type: " << ride->vehicleType
             << "\nRide Type: " << ride->rideType << "\n";
        cout << "Price of fare is: " << fare << endl;

        // Notify observers (RideRequestManager) that a new booking has been created
        bookingSubject->notify(ride);
    }

    ~BookingManager() {
        delete bookingSubject;
    }
};

// ------------------------ Main ------------------------

int main() {
    // Managers for users, drivers, and location
    userManager* um = new userManager();
    driverManager* dm = new driverManager();
    GeoLocationManager* gm = new GeoLocationManager();

    // Setup Notification System
    NotificationSubject* notifSubject = new NotificationSubject();
    notifSubject->addObserver(new UserNotificationObserver());
    notifSubject->addObserver(new DriverNotificationObserver());
    NotificationEngine* notifEngine = new NotificationEngine(notifSubject);

    // Setup Payment Gateway
    PaymentGateway* paymentGateway = new PaymentGateway();

    // Add some initial users and drivers
    um->addUser(new User("vivek", "9700407379"));
    dm->addDriver(new Driver("srinu", "SUV"));
    dm->addDriver(new Driver("raju", "Sedan"));

    // Authentication
    AuthManager* auth = new AuthManager(um, dm);

    cout << "Enter username to login: ";
    string username;
    cin >> username;
    auth->login(username);

    // Location Updates (Observer Pattern for GeoLocation)
    iLocationObserver* pollingObs = new polling(gm);
    iLocationObserver* socketObs = new socketConnection(gm);

    statusListner* status = new statusListner();
    status->addObserver(pollingObs);
    status->addObserver(socketObs);

    // Set initial locations based on context
    if (User* u = um->getUser(username)) {
        u->currentLocation = "Hyderabad";
        status->notify(u->name, u->currentLocation, u->userType);
    } else if (Driver* d = dm->getDriver(username)) {
        d->currentLocation = "Secunderabad";
        status->notify(d->name, d->currentLocation, d->userType);
    }

    // Injected dependencies for BookingManager and RideRequestManager
    IRideTypeFactorySelector* rideTypeSelector = new RideTypeFactorySelector();
    IVehicleFactorySelector* vehicleSelector = new VehicleFactorySelector();
    IPriceCalculator* priceCalc = new ConcretePriceCalculator();

    // The BookingSubject will now be created and owned by BookingManager
    BookingSubject* bookingSubjectForBM = new BookingSubject();

    IDriverAllocationOrchestrator* driverAllocOrchestrator = new ConcreteDriverAllocationOrchestrator(notifEngine);

    RideRequestManager* rideRequestManager = new RideRequestManager(notifEngine, gm, paymentGateway, driverAllocOrchestrator);
    bookingSubjectForBM->addObservers(rideRequestManager);


    BookingManager bm(rideTypeSelector, vehicleSelector, priceCalc, bookingSubjectForBM);
    bm.createBooking();

   
    delete status; 
    delete auth; 
    delete um;     
    delete dm;    
    delete gm;
    delete paymentGateway;
    delete notifEngine; 
    delete notifSubject; 

    delete rideTypeSelector;
    delete vehicleSelector;
    delete priceCalc;
    delete driverAllocOrchestrator;
    delete rideRequestManager;

    return 0;
}