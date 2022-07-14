pipeline {
  agent any
  environment {
    ARDUINO_DIRECTORIES_USER = "$WORKSPACE"
  }
  stages {
    stage('Prepare') {
      steps {
        sh 'arduino-cli core update-index'
        sh 'arduino-cli core install arduino:avr'
        sh 'arduino-cli lib install liquidcrystal'
        sh 'arduino-cli lib install "I2C-Sensor-Lib iLib"'
        sh 'arduino-cli lib install hd44780'
        sh 'arduino-cli lib install "Adafruit Unified Sensor"'
        sh 'arduino-cli lib install "DHT Sensor library"'
        sh 'arduino-cli lib install MFRC522'
        sh 'arduino-cli lib install OneWire'
        sh 'arduino-cli lib install DallasTemperature'
        sh 'arduino-cli lib install TinyGPSPlus'
        sh 'arduino-cli lib install simpleRPC'
      }
    }
    stage('Test examples') {
      steps {
        sh './make_all.sh'
      }
    }
    stage('Stage') {
      steps {
        sh 'arduino-cli compile --fqbn arduino:avr:pro:cpu=8MHzatmega328 tests/LoRaWAN-test/'
        sh 'sudo arduino-cli upload -p $WAZIDEV_PORT --fqbn arduino:avr:pro:cpu=8MHzatmega328 tests/LoRaWAN-test/'
      }
    }
  }
  post {
    always {
      junit 'results/TEST-make_all.xml'
    }
    success {
      echo 'Success!'
    }
    failure {
      echo 'Failure!'
    }
    unstable {
      echo 'Unstable'
    }
  }
}
