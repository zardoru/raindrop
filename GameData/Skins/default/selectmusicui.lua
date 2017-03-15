function activateAuto(elem)
    if elem.id == "autobtn" then
        elem.id = "autobtnac"
        Global:GetParameters(0).Autoplay = 1
    else
        elem.id = "autobtn"
        Global:GetParameters(0).Autoplay = 0
    end
end

-- checked is nil or not nil depending on whatever it is.
-- it also is on attributes rather than on the element itself (!)
function changesetting(setting, val)
    val = (val.attributes.checked ~= nil) and 1 or 0
    if setting == "upscroll" then
        Global:GetParameters(0).Upscroll = val
    elseif setting == "nofail" then
        Global:GetParameters(0).NoFail = val
    elseif setting == "random" then
        Global:GetParameters(0).Random = val
    elseif setting == "auto" then
        Global:GetParameters(0).Autoplay = val
    end
end

function changerate(v)
    rt = tonumber(v)
    if rt and rt > 0 then
        Global:GetParameters(0).Rate = rt
    end
end

function gauge_change(event)
    Global:GetParameters(0).GaugeType = tonumber(event.parameters["value"])
end

function game_change(event)
    Global:GetParameters(0).SystemType = tonumber(event.parameters["value"])
end

function hidden_change(event)
    Global:GetParameters(0).HiddenMode = tonumber(event.parameters["value"])
end

function sort_change(event)
    Global:SortWheelBy(tonumber(event.parameters["value"]))
end

function speedclass_change(event)
	System.SetConfig("SpeedClass", event.parameters["value"], "Speed")
end

function speedamt_change(event)
	System.SetConfig("SpeedAmount", event.parameters["value"], "Speed")
end

savedSpeedClass = System.ReadConfigF("SpeedClass", "Speed")
savedSpeedAmt = System.ReadConfigF("SpeedAmount", "Speed")

function onready(document)
	-- restore speed stuff from config...
	local scElem = document:GetElementById("sc" .. savedSpeedClass)
	local scaElem = document:GetElementById("spdamt")

	scaElem:SetAttribute("value", savedSpeedAmt)
	scElem:SetAttribute("selected", 1)
end