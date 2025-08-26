# ESP Side
## Requests
### Clock Sync:
```
{
  "uuid": 2,
  "request_id": "aa8ccd99-2a92-4ec7-89b9-4b574c58bd4c",
  "type": "clock_sync"
}
```

### Component Update:
```
{
  "uuid": 2,
  "request_id": "aa8ccd99-2a92-4ec7-89b9-4b574c58bd4c",
  "type": "component_update",
  "component": "43b418ed-35b4-40e0-b5bd-1290fcaa527e",
  "data": {
    "temp": 23,
    "enabled": true
  }
}
```

### Component Fetch:
```
{
  "uuid": 2,
  "request_id": "aa8ccd99-2a92-4ec7-89b9-4b574c58bd4c",
  "type": "component_fetch",
  "component": "43b418ed-35b4-40e0-b5bd-1290fcaa527e"
}
```

# Components

## Climate Control
### States
```
temp: <int>
enabled: <boolean>
```
### Config
```
fan: <component_uuid>
ac: <component_uuid>
```
### Settings
```
# TODO
```

## Cover
### States
```
setpoint: <int>
is_at_setpoint: <boolean>
```

### Config
```
entity_id: <string>
```

### Settings
```
# I don't think any settings are needed, but this can be reconsidered.
```

## AC
### States
```
temp: <int>
fan_level: <int>
power: <boolean>
```
### Config
```
# TODO
```

### Settings
```
# TODO
```

## Fan
### States
```
fan_level: <int>
fan_power: <boolean>
light_power: <boolean>
```

### Config
```
# TODO
```

### Settings
```
# TODO
```

## Light
### States
```
power: <boolean>
```
### Config
```
entity_id: <string>
```
### Settings
```
# TODO
```

## Lock
### States
```
state: <locked/unlocked/unlocking/locking>
```
### Config
```
entity_id: <string>
```
### Settings
```
# TODO
```

## Alarm
### States
```
state: <disarmed/armed_away/armed_home/arming>
```
### Config
```
entity_id: <string>
```
### Settings
```
# TODO
```

## Vacuum
### States
```
state: <docked/cleaning>
```
### Config
```
entity_id: <string>
```
### Settings
```
# TODO
```