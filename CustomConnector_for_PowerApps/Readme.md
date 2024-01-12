# Azure IoT Hub Custom Connector
This is a custom connector for connect to Azure IoT Hub for Power Apps or Power Automate.  


# Features
- Get a list of devices.
- Send messages from the cloud to the device.
- Get device twin properties.
- Invoke direct method of the device.

# Prerequisites
- You need an Azure IoT Hub resource and device, and an IoT device that implements the IoT Hub SDK.
- Save the “Azure-IoT-Hub-Connector.swagger.json” file.
- Obtain a SAS token for the IoT Hub service using the Azure IoT Hub extension in VSCode. <br> 
 ※Make sure not to confuse it with the device SAS token.

# How to set up
1. Select Create a new custom connector and select Import an OpenAPI file.
1. Import the “Azure-IoT-Hub-Connector.swagger.json” file.
Modify the host “Your-Host-Name.azure-devices.net” to match your own IoT Hub.
1. Save the custom connector.
1. In the create connection screen, set the SAS token as the API Key.
1. Test the connector and check its operation.

  
# Note
You cannot get the telemetry sent from the device.   
```GET devices/{id}/messages/events``` endpoint is not available with the HTTPS protocol.