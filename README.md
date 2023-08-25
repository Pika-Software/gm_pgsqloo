# gm_pgsqloo
PostgreSQL binary module for Garry's Mod

## Example
```lua
require "pgsqloo"

local db = pgsqloo.Connection("postgresql://postgres@localhost/postgres?application_name=pgsqloo")

function db:OnConnected(err)
    if err then
        print("Failed to connect to database:", err)
    else
        print("Successfully connected to database")
    end
end

db:Connect()

db:Query("SELECT * FROM users", function(err, result)
    if err then
        print("Failed to get all users:", err)
    else
        print("Users:")
        PrintTable(result)
    end
end)

local task = db:Query("SELECT * FROM stats WHERE level = $1 AND money > $2", {80, 99999}):Wait()
if task:IsSuccess() then
    PrintTable(task:GetResult())
else
    print(task:GetError())
end
```

## API
### Global API
```lua
-- See https://www.postgresql.org/docs/current/libpq-connect.html#LIBPQ-CONNSTRING
pgsqloo.Connection(url: string) -> PostgreConnection
```
### PostgreConnection
```lua
PostgreConnection:Connect() -> PostgreTask<nil>
PostgreConnection:Query(sql: string, args: table?, callback: function(err: string?, result: Result?)?) -> PostgreTask<Result>
PostgreConnection:PreparedQuery(name: string, args: table?, callback: function(err: string?, result: Result?)?) -> PostgreTask<Result>
PostgreConnection:Prepare(name: string, sql: string)
PostgreConnection:Unprepare(name: string)
PostgreConnection:Disconnect(wait: bool = false)
PostgreConnection:IsConnected() -> bool
PostgreConnection:IsConnecting() -> bool
PostgreConnection:Database() -> string
PostgreConnection:Username() -> string
PostgreConnection:Hostname() -> string
PostgreConnection:Port() -> string
PostgreConnection:BackendPID() -> number
PostgreConnection:ProtocolVersion() -> number
PostgreConnection:ServerVersion() -> number
PostgreConnection:GetClientEncoding() -> string
PostgreConnection:SetClientEncoding(encoding: string)
PostgreConnection:EncodingID() -> number

-- Events
PostgreConnection.OnConnected(conn: PostgreConnection, err: string?)
```
### PostgreTask
```lua
PostgreTask:Wait() -> PostgreTask
PostgreTask:IsSuccess() -> bool
PostgreTask:IsError() -> bool
PostgreTask:IsProcessing() -> bool
PostgreTask:GetResult() -> ResultType/nil
PostgreTask:GetError() -> string/nil
```