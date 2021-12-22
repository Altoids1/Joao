//WARNING!!!
//THIS CODE MUST BE EXECUTABLE AS BOTH RUST AND C++!!
//BOTH AUXTOOLS AND JOAO INCLUDE THIS FILE!

//Note: These enums have to be storable in a u8/uint8_t
//Rust doesn't really let you define the underlying type of these things,
//so please pinky-promise to ensure these values all fit in [0,255].
enum joaoErrorCode
{
    NoError = 0,
    ScannerError,
    ParserError,
    TransferError, //An error in reading/writing the packet or script.
    //These next 3 are all errors that can occur during interpretation.
    RuntimeError,
    FatalError,
    UnknownError
};