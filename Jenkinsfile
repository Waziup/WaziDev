pipeline {
  agent any
  stages {
    stage('Build examples') {
      steps {
        sh './make_all.sh'
        }
      }
    }
  }
  post {
    always {
      junit 'tests/report.xml'
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
