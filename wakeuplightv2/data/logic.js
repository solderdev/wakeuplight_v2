class Logic {
    constructor() {
        console.log('Creating Logic')

        this.alarm_time = "11:11"
        this.alarm_weekend = false
        this.fade_minutes = 20
        this.snooze_minutes = 30
        this.mode = 2
        this.duty_max = 99.0
        this.duty_lights_on = 60.0
        this.current_percent = 1.1
        this.host_uri = window.location.origin + "/"
        var evsource = new EventSource('/events');
        evsource.addEventListener('currentpercent', function(e) {
            console.log('SSE currentpercent', e.data)
            this.current_percent = parseFloat(e.data)
            document.getElementById('mode_percent').innerHTML = this.current_percent + ' %'
        })
        evsource.addEventListener('currenttime', function(e) {
            console.log('SSE currenttime', e.data)
            document.getElementById('mode_time').innerHTML = e.data
        })

        console.log("URI: " + this.host_uri)

        document.getElementById('lights_on_button_id').addEventListener(
            "click", event => this.onButtonClick(event, "lights_on"), true)

        document.getElementById('alarm_off_button_id').addEventListener(
            "click", event => this.onButtonClick(event, "alarm_off"), true)

        document.getElementById('alarm_on_button_id').addEventListener(
            "click", event => this.onButtonClick(event, "alarm_on"), true)

        document.getElementById('alarm_time_form_id').addEventListener(
            "focusout", event => this.onSubmitAlarmTime(event), true)
        document.getElementById('alarm_time_form_id').addEventListener(
            "submit", event => this.onSubmitPreventDefault(event), true)

        document.getElementById('alarm_weekend_form_id').addEventListener(
            "change", event => this.onSubmitAlarmWeekend(event), true)
        document.getElementById('alarm_weekend_form_id').addEventListener(
            "submit", event => this.onSubmitPreventDefault(event), true)

        document.getElementById('fade_minutes_form_id').addEventListener(
            "change", event => this.onSubmitFadeMinutes(event), true)
        document.getElementById('fade_minutes_form_id').addEventListener(
            "submit", event => this.onSubmitPreventDefault(event), true)

        document.getElementById('snooze_minutes_form_id').addEventListener(
            "change", event => this.onSubmitSnoozeMinutes(event), true)
        document.getElementById('snooze_minutes_form_id').addEventListener(
            "submit", event => this.onSubmitPreventDefault(event), true)

        document.getElementById('duty_lights_on_form_id').addEventListener(
            "change", event => this.onSubmitDutyLightsOn(event), true)
        document.getElementById('duty_lights_on_form_id').addEventListener(
            "submit", event => this.onSubmitPreventDefault(event), true)

        this.fetchParameters()
    }

    modeToString(mode) {
        // see AlarmMode_t
        if (mode === 0) return "Lights ON"
        if (mode === 1) return "Alarm ON"
        if (mode === 2) return "Alarm OFF"
        return "undefined"
    }

    parseParameters(data) {
        console.log('got Parameters: ' + data)
        let params = data.split(" ")

        this.alarm_time = params[0]
        this.alarm_weekend = JSON.parse(params[1]) // true / false
        this.fade_minutes = parseInt(params[2])
        this.mode = parseInt(params[3])
        this.duty_max = parseFloat(params[4])
        this.duty_lights_on = parseFloat(params[5])
        this.current_percent = parseFloat(params[6])
        this.snooze_minutes = parseInt(params[7])

        console.log('update: alarm_time=' + this.alarm_time +
            ' alarm_weekend=' + this.alarm_weekend +
            ' fade_minutes=' + this.fade_minutes +
            ' mode=' + this.mode +
            ' duty_max=' + this.duty_max +
            ' duty_lights_on=' + this.duty_lights_on +
            ' current_percent=' + this.current_percent + 
            ' snooze_minutes=' + this.snooze_minutes)

        document.getElementById('alarm_time_input_id').value = this.alarm_time
        document.getElementById('alarm_weekend_input_id').checked = this.alarm_weekend
        document.getElementById('fade_minutes_input_id').value = this.fade_minutes
        document.getElementById('mode_id').innerHTML = this.modeToString(this.mode)
        document.getElementById('mode_percent').innerHTML = this.current_percent + ' %'
        document.getElementById('snooze_minutes_input_id').value = this.snooze_minutes
        document.getElementById('duty_lights_on_input_id').value = this.duty_lights_on / this.duty_max * 100
        document.getElementById('duty_lights_on_input_id').style.background =
            `linear-gradient(to right,#4BD663,#4BD663 ${this.duty_lights_on}%,#eee ${this.duty_lights_on}%)`;
        if (this.mode === 0) {
            document.getElementById('lights_on_button_id').style.backgroundColor = "#4CAF50";
            document.getElementById('alarm_off_button_id').style.backgroundColor = "lightgray";
            document.getElementById('alarm_on_button_id').style.backgroundColor = "lightgray";
        } else if (this.mode === 1) {
            document.getElementById('lights_on_button_id').style.backgroundColor = "lightgray";
            document.getElementById('alarm_off_button_id').style.backgroundColor = "lightgray";
            document.getElementById('alarm_on_button_id').style.backgroundColor = "#4CAF50";
        } else if (this.mode === 2) {
            document.getElementById('lights_on_button_id').style.backgroundColor = "lightgray";
            document.getElementById('alarm_off_button_id').style.backgroundColor = "#4CAF50";
            document.getElementById('alarm_on_button_id').style.backgroundColor = "lightgray";
        }
    }

    fetchParameters() {
        console.log('fetchParameters()')

        fetch(this.host_uri + "parameters", {
            method: 'GET',
            headers: {'Content-Type': 'text/plain'},
            body: null,
        })
            .then(response => response.text())
            .then(data => {this.parseParameters(data)})
            // TODO lock inputs until update is complete
    }

    onSubmitPreventDefault(event) {
        console.log("onSubmitPreventDefault()")
        event.preventDefault()
    }

    post_and_fetch(endpoint, body_data) {
        document.getElementById('mode_id').innerHTML = "updating ..."
        
        fetch(this.host_uri + endpoint, {
            method: 'POST',
            headers: {'Content-Type': 'text/plain'},
            body: body_data,
        })
            .then(response => response.text())
            .then(data => {this.parseParameters(data)})
    }

    onButtonClick(callback, name) {
        console.log("onButtonClick()")

        let endpoint = "undefined"
        if (name === "alarm_off") endpoint = "alarm_off"
        else if (name === "alarm_on") endpoint = "alarm_on"
        else if (name === "lights_on") endpoint = "lights_on"
        else {
            console.error("ERROR: undefined button callback name: " + name)
            return
        }
        this.post_and_fetch(endpoint, null)
    }

    onSubmitAlarmTime(event) {
        console.log("onSubmitAlarmTime()")
        event.preventDefault()
        let alarm_time = document.getElementById('alarm_time_input_id').value
        console.log("Alarm Time: " + alarm_time.toString())
        this.post_and_fetch("set_alarm_time", alarm_time.toString() + '\0')
    }

    onSubmitAlarmWeekend(event) {
        console.log("onSubmitAlarmWeekend()")
        event.preventDefault()
        let alarm_weekend = document.getElementById('alarm_weekend_input_id').checked
        console.log("Alarm Weekend: " + alarm_weekend.toString())
        this.post_and_fetch("set_alarm_weekend", alarm_weekend.toString() + '\0')
    }

    onSubmitFadeMinutes(event) {
        console.log("onSubmitFadeMinutes()")
        event.preventDefault()
        let fade_m = Math.max(1, parseInt(document.getElementById('fade_minutes_input_id').value))
        fade_m = Math.min(120, fade_m)
        console.log("Fade Minutes: " + fade_m.toString())
        this.post_and_fetch("set_fade_minutes", fade_m.toString() + '\0')
    }

    onSubmitSnoozeMinutes(event) {
        console.log("onSubmitSnoozeMinutes()")
        event.preventDefault()
        let snooze_m = Math.max(0, parseFloat(document.getElementById('snooze_minutes_input_id').value))
        snooze_m = Math.min(200, snooze_m)
        console.log("Snooze Minutes: " + snooze_m.toString())
        this.post_and_fetch("set_snooze_minutes", snooze_m.toString() + '\0')
    }

    onSubmitDutyLightsOn(event) {
        console.log("onSubmitDutyLightsOn()")
        event.preventDefault()
        let duty = Math.max(0.001, parseFloat(document.getElementById('duty_lights_on_input_id').value))
        duty = Math.min(100.0, duty)
    	document.getElementById('duty_lights_on_input_id').style.background =
            `linear-gradient(to right,#4BD663,#4BD663 ${duty}%,#eee ${duty}%)`;
        console.log("Lights-on Duty: " + duty.toString())
        this.post_and_fetch("set_duty_lights_on", duty.toString() + '\0')
    }
}

const logic = new Logic()
