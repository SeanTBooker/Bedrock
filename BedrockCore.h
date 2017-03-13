#pragma once
#include <sqlitecluster/SQLiteCore.h>
class BedrockCommand;
class BedrockServer;

class BedrockCore : public SQLiteCore {
  public:
    BedrockCore(SQLite& db, const BedrockServer& server);

    // Peek lets you pre-process a command. It will be called on each command before `process` is called on the same
    // command, and it *may be called multiple times*. Preventing duplicate actions on calling peek multiple times is
    // up to the implementer, and may happen *across multiple servers*. I.e., a slave server may call `peek`, and on
    // its returning false, the command will be escalated to the master server, where `peek` will be called again.It
    // should be considered an error to modify the DB from inside `peek`.
    // Returns a boolean value of `true` if the command is complete and it's `response` field can be returned to the
    // caller. Returns `false` if the command will need to be passed to `process` to complete handling the command.
    bool peekCommand(BedrockCommand& command);

    // Process is the follow-up to `peek` if peek was insufficient to handle the command. It will only ever be called
    // on the master node, and should always be able to resolve the command completely. 
    // When a command is passed to commit, the caller will have already begun a database transaction with either `BEGIN
    // TRANSACTION` or `BEGIN CONCURRENT`, and it's up to `process` to add the rest of the transaction, without
    // performing a `ROLLBACK` or `COMMIT`, which will be handled by the caller.
    // It returns `true` if it has modified the database and requires the caller to perform a `commit`, and `false` if
    // no changes requiring a commit have been made.
    // Upon being returned `false`, the caller will perform a `ROLLBACK` of the empty transaction, and will not
    // replicate the transaction to slave nodes.
    // Upon being returned `true`, the caller will attempt to perform a `COMMIT` and replicate the transaction to slave
    // nodes. It's allowable for this `COMMIT` to fail, in which case this command will be passed to process again in
    // the future.
    bool processCommand(BedrockCommand& command);

    // Lets plugins upgrade the database to conform to whatever schema they require.
    void upgradeDatabase();

  private:
    void _handleCommandException(BedrockCommand& command, const string& e, bool wasProcessing);
    const BedrockServer& _server;
};